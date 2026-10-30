#include "semantic/semantic.hpp"

#include "builtins.hpp"
#include "semantic/semantic_internal.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nexus {

    std::optional<Type> SemanticAnalyzer::provenanceType(
        const Expr* e, const std::unordered_map<std::string, Symbol>& env) const {
        if (!e)
            return std::nullopt;

        if (dynamic_cast<const IntExpr*>(e))
            return Type::i64();
        if (dynamic_cast<const FloatExpr*>(e))
            return Type::f64();
        if (dynamic_cast<const BoolExpr*>(e))
            return Type::boolean();
        if (dynamic_cast<const StringExpr*>(e))
            return Type::string();
        if (const auto* variable = dynamic_cast<const VarExpr*>(e)) {
            const auto it = env.find(variable->name);
            return it == env.end() ? std::nullopt : std::optional<Type>(it->second.type);
        }
        if (const auto* object = dynamic_cast<const StructInitExpr*>(e))
            return Type::named(object->typeName);
        if (const auto* variant = dynamic_cast<const EnumVariantExpr*>(e))
            return Type::enumerated(variant->enumName);
        if (const auto* array = dynamic_cast<const ArrayExpr*>(e)) {
            if (array->elements.empty())
                return std::nullopt;
            const auto element = provenanceType(array->elements.front().get(), env);
            return element ? std::optional<Type>(Type::array(*element, array->elements.size()))
                           : std::nullopt;
        }
        if (const auto* unary = dynamic_cast<const UnaryExpr*>(e)) {
            const auto inner = provenanceType(unary->rhs.get(), env);
            if (!inner)
                return std::nullopt;
            if (unary->op == TokenKind::Ampersand)
                return Type::ptr(*inner);
            if (unary->op == TokenKind::Star &&
                inner->kind == TypeKind::Pointer && inner->pointee)
                return *inner->pointee;
            return inner;
        }
        if (const auto* member = dynamic_cast<const MemberExpr*>(e)) {
            const auto objectType = provenanceType(member->object.get(), env);
            if (!objectType || objectType->kind != TypeKind::Struct)
                return std::nullopt;
            const auto declaration = structs_.find(objectType->name);
            if (declaration == structs_.end())
                return std::nullopt;
            const auto* field = fieldOf(*declaration->second, member->member);
            return field ? std::optional<Type>(field->type) : std::nullopt;
        }
        if (const auto* index = dynamic_cast<const IndexExpr*>(e)) {
            const auto objectType = provenanceType(index->object.get(), env);
            if (!objectType || objectType->kind != TypeKind::Array || !objectType->element)
                return std::nullopt;
            return *objectType->element;
        }
        if (const auto* call = dynamic_cast<const CallExpr*>(e)) {
            if (const auto builtin = builtinSignature(call->callee))
                return builtin->ret;
            const auto function = funcs_.find(call->callee);
            return function == funcs_.end() ? std::nullopt
                                            : std::optional<Type>(function->second->ret);
        }
        if (const auto* attempt = dynamic_cast<const TryExpr*>(e)) {
            const auto inner = provenanceType(attempt->expr.get(), env);
            if (!inner || inner->kind != TypeKind::Enum)
                return std::nullopt;
            const auto declaration = enums_.find(inner->name);
            if (declaration == enums_.end())
                return std::nullopt;
            for (const auto& variant : declaration->second->variants) {
                if ((variant.name == "Some" || variant.name == "Ok") && variant.payload)
                    return *variant.payload;
            }
            return std::nullopt;
        }
        return std::nullopt;
    }

    std::optional<BorrowOrigin> SemanticAnalyzer::pointerOrigin(
        const Expr* e, const std::unordered_map<std::string, Symbol>& env) const {
        if (!e)
            return std::nullopt;

        if (const auto* variable = dynamic_cast<const VarExpr*>(e)) {
            const auto it = env.find(variable->name);
            return it == env.end() ? std::nullopt : it->second.borrowedFrom;
        }

        if (const auto* unary = dynamic_cast<const UnaryExpr*>(e)) {
            if (unary->op == TokenKind::Ampersand) {
                const auto* root = lvalueRootVariable(unary->rhs.get());
                if (!root)
                    return std::nullopt;
                const auto it = env.find(root->name);
                if (it == env.end())
                    return std::nullopt;
                return BorrowOrigin{it->second.scopeDepth, root->name};
            }

            // Dereferencing a pointer-to-pointer reads the pointer value stored
            // in the referenced binding, so preserve that binding's provenance.
            if (unary->op == TokenKind::Star) {
                const auto storage = pointerOrigin(unary->rhs.get(), env);
                if (storage) {
                    const auto it = env.find(storage->binding);
                    if (it != env.end())
                        return it->second.borrowedFrom;
                }
                return std::nullopt;
            }
        }

        std::optional<BorrowOrigin> result;
        const auto merge = [&](const std::optional<BorrowOrigin>& candidate) {
            if (candidate && (!result || candidate->scopeDepth > result->scopeDepth))
                result = candidate;
        };

        if (const auto* call = dynamic_cast<const CallExpr*>(e)) {
            const auto summary = returnedBorrowParams_.find(call->callee);
            if (summary != returnedBorrowParams_.end()) {
                for (const auto index : summary->second) {
                    if (index < call->args.size())
                        merge(pointerOrigin(call->args[index].get(), env));
                }
            }
        } else if (const auto* array = dynamic_cast<const ArrayExpr*>(e)) {
            if (!array->elements.empty()) {
                const auto elementType = provenanceType(array->elements.front().get(), env);
                if (elementType && typeMayContainPointer(*elementType)) {
                    for (const auto& element : array->elements)
                        merge(pointerOrigin(element.get(), env));
                }
            }
        } else if (const auto* object = dynamic_cast<const StructInitExpr*>(e)) {
            const auto declaration = structs_.find(object->typeName);
            if (declaration != structs_.end()) {
                for (const auto& [name, value] : object->fields) {
                    const auto* field = fieldOf(*declaration->second, name);
                    if (field && typeMayContainPointer(field->type))
                        merge(pointerOrigin(value.get(), env));
                }
            }
        } else if (const auto* variant = dynamic_cast<const EnumVariantExpr*>(e)) {
            const auto declaration = enums_.find(variant->enumName);
            if (declaration != enums_.end()) {
                for (const auto& item : declaration->second->variants) {
                    if (item.name == variant->variant && item.payload &&
                        typeMayContainPointer(*item.payload)) {
                        merge(pointerOrigin(variant->payload.get(), env));
                        break;
                    }
                }
            }
        } else if (const auto* member = dynamic_cast<const MemberExpr*>(e)) {
            const auto memberType = provenanceType(member, env);
            if (memberType && typeMayContainPointer(*memberType))
                merge(pointerOrigin(member->object.get(), env));
        } else if (const auto* index = dynamic_cast<const IndexExpr*>(e)) {
            const auto elementType = provenanceType(index, env);
            if (elementType && typeMayContainPointer(*elementType))
                merge(pointerOrigin(index->object.get(), env));
        }

        return result;
    }

    Type SemanticAnalyzer::expr(Expr* e, std::unordered_map<std::string, Symbol>& env) {
        try {
            return exprImpl(e, env);
        } catch (const std::exception& error) {
            const std::string message = error.what();
            if (e && e->span.valid && message.rfind(" at ") == std::string::npos) {
                throw std::runtime_error(message + sourceLocation(e->span));
            }
            throw;
        }
    }

    Type SemanticAnalyzer::exprImpl(Expr* e, std::unordered_map<std::string, Symbol>& env) {

        if (dynamic_cast<IntExpr*>(e))
            return Type::i64();
        if (dynamic_cast<FloatExpr*>(e))
            return Type::f64();
        if (dynamic_cast<BoolExpr*>(e))
            return Type::boolean();
        if (dynamic_cast<StringExpr*>(e))
            return Type::string();

        if (auto* v = dynamic_cast<VarExpr*>(e)) {
            if (v->name == "_")
                throw std::runtime_error("wildcard '_' is only valid in match patterns");
            auto it = env.find(v->name);
            if (it == env.end())
                throw std::runtime_error("unknown variable: " + v->name);
            return it->second.type;
        }
        if (auto* ev = dynamic_cast<EnumVariantExpr*>(e)) {
            const auto& en = getEnum(ev->enumName);
            const EnumVariant* selected = nullptr;
            for (const auto& variant : en.variants) {
                if (variant.name == ev->variant) {
                    selected = &variant;
                    break;
                }
            }
            if (!selected)
                throw std::runtime_error("unknown enum variant: " + ev->enumName +
                                         "::" + ev->variant);

            const std::string qualifiedName = ev->enumName + "::" + ev->variant;
            if (selected->payload) {
                if (!ev->payload)
                    throw std::runtime_error("enum variant '" + qualifiedName +
                                             "' requires a payload of type '" +
                                             describeType(*selected->payload) + "'");
                const auto actual = expr(ev->payload.get(), env);
                if (!sameType(actual, *selected->payload))
                    throw std::runtime_error("enum variant '" + qualifiedName +
                                             "' expects payload '" +
                                             describeType(*selected->payload) + "' but got '" +
                                             describeType(actual) + "'");
            } else if (ev->payload) {
                throw std::runtime_error("enum variant '" + qualifiedName +
                                         "' does not accept a payload");
            }
            return Type::enumerated(ev->enumName);
        }
        if (auto* t = dynamic_cast<TryExpr*>(e)) {
            const auto inner = expr(t->expr.get(), env);
            if (inner.kind != TypeKind::Enum)
                throw std::runtime_error("operator '?' requires an enum with Some/None or Ok/Err variants");

            const auto& en = getEnum(inner.name);
            const EnumVariant* success = nullptr;
            const EnumVariant* failure = nullptr;
            for (const auto& variant : en.variants) {
                if (variant.name == "Some" || variant.name == "Ok") {
                    if (success)
                        throw std::runtime_error("operator '?' is ambiguous for enum '" + en.name + "'");
                    success = &variant;
                }
                if (variant.name == "None" || variant.name == "Err") {
                    if (failure)
                        throw std::runtime_error("operator '?' is ambiguous for enum '" + en.name + "'");
                    failure = &variant;
                }
            }
            if (!success || !success->payload || !failure)
                throw std::runtime_error("operator '?' requires an enum with Some(T)/None or Ok(T)/Err variants");
            if ((success->name == "Some" && failure->name != "None") ||
                (success->name == "Ok" && failure->name != "Err"))
                throw std::runtime_error("operator '?' requires matching Some/None or Ok/Err variants");
            if (currentFunctionReturnType_.kind != TypeKind::Enum ||
                currentFunctionReturnType_.name != inner.name)
                throw std::runtime_error("operator '?' requires the enclosing function to return '" +
                                         describeType(inner) + "'");
            return *success->payload;
        }
        if (auto* a = dynamic_cast<ArrayExpr*>(e)) {
            if (a->elements.empty())
                throw std::runtime_error(
                    "empty array literals are not supported; add at least one element");
            auto t = expr(a->elements[0].get(), env);
            for (size_t i = 1; i < a->elements.size(); ++i) {
                auto actual = expr(a->elements[i].get(), env);
                if (!sameType(t, actual))
                    throw std::runtime_error("array element " + std::to_string(i + 1) +
                                             " expects '" + describeType(t) + "' but got '" +
                                             describeType(actual) + "'");
            }
            return Type::array(t, a->elements.size());
        }
        if (auto* ix = dynamic_cast<IndexExpr*>(e)) {
            auto ot = expr(ix->object.get(), env);
            if (ot.kind != TypeKind::Array || !ot.element)
                throw std::runtime_error("indexing requires an array");
            if (expr(ix->index.get(), env).kind != TypeKind::I64)
                throw std::runtime_error("array index must be i64");
            if (constantIndexOutOfBounds(ix->index.get(), ot.arraySize))
                throw std::runtime_error("array index is out of bounds");
            return *ot.element;
        }
        if (auto* u = dynamic_cast<UnaryExpr*>(e)) {
            auto t = expr(u->rhs.get(), env);
            if (u->op == TokenKind::Minus && (t.kind != TypeKind::I64 && t.kind != TypeKind::F64))
                throw std::runtime_error("unary '-' requires number");
            if (u->op == TokenKind::Bang && t.kind != TypeKind::Bool)
                throw std::runtime_error("'!' requires bool");
            if (u->op == TokenKind::Ampersand) {
                const auto* root = lvalueRootVariable(u->rhs.get());
                if (!root)
                    throw std::runtime_error("'&' requires a variable, field, or array element");
                const auto it = env.find(root->name);
                if (it == env.end())
                    throw std::runtime_error("unknown variable: " + root->name);
                if (!it->second.mut)
                    throw std::runtime_error(
                        "'&' requires mutable storage; declare the owning variable with 'let mut'");
                return Type::ptr(t);
            }
            if (u->op == TokenKind::Star) {
                if (t.kind != TypeKind::Pointer || !t.pointee)
                    throw std::runtime_error("'*' requires pointer");
                return *t.pointee;
            }
            return t;
        }
        if (auto* b = dynamic_cast<BinaryExpr*>(e)) {

            auto l = expr(b->lhs.get(), env), r = expr(b->rhs.get(), env);

            // Smart string concatenation: "text " + 42 becomes "text " + str_i64(42).
            if (b->op == TokenKind::Plus && !sameType(l, r)) {
                auto toStrFn = [](const Type& t) -> const char* {
                    if (t.kind == TypeKind::I64)
                        return "str_i64";
                    if (t.kind == TypeKind::F64)
                        return "str_f64";
                    if (t.kind == TypeKind::Bool)
                        return "str_bool";
                    return nullptr;
                };
                if (l.kind == TypeKind::String) {
                    if (const char* fn = toStrFn(r)) {
                        std::vector<Expr::Ptr> args;
                        args.push_back(std::move(b->rhs));
                        b->rhs = std::make_unique<CallExpr>(fn, std::move(args));
                        r = l;
                    }
                } else if (r.kind == TypeKind::String) {
                    if (const char* fn = toStrFn(l)) {
                        std::vector<Expr::Ptr> args;
                        args.push_back(std::move(b->lhs));
                        b->lhs = std::make_unique<CallExpr>(fn, std::move(args));
                        l = r;
                    }
                }
            }

            if (!sameType(l, r))
                throw std::runtime_error("binary operands have different types: left is '" +
                                         describeType(l) + "', right is '" + describeType(r) + "'");

            switch (b->op) {

            case TokenKind::Plus:
                if (l.kind == TypeKind::String || l.kind == TypeKind::I64 ||
                    l.kind == TypeKind::F64)
                    return l;
                throw std::runtime_error("operator + requires numbers or strings");

            case TokenKind::Minus:
            case TokenKind::Star:
            case TokenKind::Slash:
            case TokenKind::Percent:
                if (l.kind != TypeKind::I64 && l.kind != TypeKind::F64)
                    throw std::runtime_error("arithmetic requires numeric types");
                // Reject a literal zero divisor at compile time; otherwise the
                // program dies with a hardware exception instead of a diagnostic.
                if (l.kind == TypeKind::I64 &&
                    (b->op == TokenKind::Slash || b->op == TokenKind::Percent)) {
                    if (const auto divisor = constantIntegerValue(b->rhs.get());
                        divisor && *divisor == 0)
                        throw std::runtime_error("division by zero");
                }
                return l;

            case TokenKind::EqualEqual:
            case TokenKind::BangEqual:
                if (l.kind == TypeKind::String || l.kind == TypeKind::I64 ||
                    l.kind == TypeKind::F64 || l.kind == TypeKind::Bool || l.kind == TypeKind::Enum)
                    return Type::boolean();
                throw std::runtime_error("equality is unsupported for this type");

            case TokenKind::Less:
            case TokenKind::LessEqual:
            case TokenKind::Greater:
            case TokenKind::GreaterEqual:
                if (l.kind == TypeKind::String)
                    throw std::runtime_error("string ordering is not supported; use == or !=");
                if (l.kind == TypeKind::I64 || l.kind == TypeKind::F64)
                    return Type::boolean();
                throw std::runtime_error("comparison requires numeric types");

            case TokenKind::AndAnd:
            case TokenKind::OrOr:
                if (l.kind != TypeKind::Bool)
                    throw std::runtime_error("logical operators require bool");
                return Type::boolean();

            case TokenKind::Ampersand:
            case TokenKind::Pipe:
            case TokenKind::Caret:
            case TokenKind::ShiftLeft:
            case TokenKind::ShiftRight:
                if (l.kind != TypeKind::I64)
                    throw std::runtime_error("bitwise operators require i64 operands");
                return Type::i64();

            default:
                break;
            }
        }
        if (auto* c = dynamic_cast<CallExpr*>(e)) {
            if (c->callee == "print") {
                if (c->args.size() != 1)
                    throw std::runtime_error("print expects one argument");
                auto t = expr(c->args[0].get(), env);
                if (t.kind != TypeKind::I64 && t.kind != TypeKind::F64 &&
                    t.kind != TypeKind::Bool && t.kind != TypeKind::String)
                    throw std::runtime_error("print: unsupported type");
                return Type::void_();
            }
            if (auto bi = builtinSignature(c->callee)) {
                if (bi->params.size() != c->args.size())
                    throw std::runtime_error("wrong argument count for builtin " + c->callee);
                for (size_t i = 0; i < c->args.size(); ++i) {
                    auto actual = expr(c->args[i].get(), env);
                    if (!sameType(actual, bi->params[i]))
                        throw std::runtime_error("argument " + std::to_string(i + 1) +
                                                 " of builtin '" + c->callee + "' expects '" +
                                                 describeType(bi->params[i]) + "' but got '" +
                                                 describeType(actual) + "'");
                }
                return bi->ret;
            }
            auto it = funcs_.find(c->callee);
            if (it == funcs_.end())
                throw std::runtime_error("unknown function: " + c->callee);
            auto* f = it->second;
            if (f->params.size() != c->args.size())
                throw std::runtime_error("wrong argument count for " + c->callee);
            for (size_t i = 0; i < c->args.size(); ++i) {
                auto actual = expr(c->args[i].get(), env);
                if (!sameType(actual, f->params[i].type))
                    throw std::runtime_error("argument " + std::to_string(i + 1) + " ('" +
                                             f->params[i].name + "') of '" + c->callee +
                                             "' expects '" + describeType(f->params[i].type) +
                                             "' but got '" + describeType(actual) + "'");
            }
            return f->ret;
        }
        if (auto* m = dynamic_cast<MemberExpr*>(e)) {
            auto t = expr(m->object.get(), env);
            if (t.kind != TypeKind::Struct)
                throw std::runtime_error("member access requires struct");
            auto& s = getStruct(t.name);
            auto* f = fieldOf(s, m->member);
            if (!f)
                throw std::runtime_error("unknown field '" + m->member + "' on " + t.name);
            return f->type;
        }
        if (auto* si = dynamic_cast<StructInitExpr*>(e)) {
            auto& s = getStruct(si->typeName);
            std::unordered_map<std::string, bool> supplied;
            for (const auto& [name, value] : si->fields) {
                (void)value;
                if (!supplied.emplace(name, true).second)
                    throw std::runtime_error("duplicate field initializer '" + name + "' in '" +
                                             si->typeName + "'");
                if (!fieldOf(s, name))
                    throw std::runtime_error("unknown field '" + name + "' in initializer for '" +
                                             si->typeName + "'");
            }
            for (const auto& field : s.fields) {
                if (!supplied.contains(field.name))
                    throw std::runtime_error("missing field '" + field.name +
                                             "' in initializer for '" + si->typeName + "'");
            }
            for (auto& [name, x] : si->fields) {
                auto* f = fieldOf(s, name);
                auto actual = expr(x.get(), env);
                if (!sameType(actual, f->type))
                    throw std::runtime_error("field '" + name + "' of '" + si->typeName +
                                             "' expects '" + describeType(f->type) + "' but got '" +
                                             describeType(actual) + "'");
            }
            return Type::named(si->typeName);
        }
        throw std::runtime_error("unsupported expression");
    }

} // namespace nexus
