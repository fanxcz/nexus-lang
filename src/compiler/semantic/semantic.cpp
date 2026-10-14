#include "semantic/semantic.hpp"

#include "builtins.hpp"

#include <functional>
#include <stdexcept>
#include <sstream>
#include <unordered_map>
namespace nexus {

    const StructDecl& SemanticAnalyzer::getStruct(const std::string& n) const {
        auto it = structs_.find(n);
        if (it == structs_.end())
            throw std::runtime_error("unknown struct: " + n);
        return *it->second;
    }
    const EnumDecl& SemanticAnalyzer::getEnum(const std::string& n) const {
        auto it = enums_.find(n);
        if (it == enums_.end())
            throw std::runtime_error("unknown enum: " + n);
        return *it->second;
    }
    static const StructField* fieldOf(const StructDecl& s, const std::string& n) {
        for (auto& f : s.fields)
            if (f.name == n)
                return &f;
        return nullptr;
    }

    static std::string describeType(const Type& type) {
        switch (type.kind) {
        case TypeKind::I64:
            return "i64";
        case TypeKind::F64:
            return "f64";
        case TypeKind::Bool:
            return "bool";
        case TypeKind::String:
            return "string";
        case TypeKind::Void:
            return "void";
        case TypeKind::Unknown:
            return "unknown";
        case TypeKind::Struct:
            return type.name;
        case TypeKind::Enum:
            return "enum " + type.name;
        case TypeKind::Pointer:
            return type.pointee ? "*" + describeType(*type.pointee) : "*unknown";
        case TypeKind::Array:
            return type.element ? "[" + describeType(*type.element) + "; " +
                                      std::to_string(type.arraySize) + "]"
                                : "[unknown]";
        }
        return "unknown";
    }

    // Conservative control-flow check: only accept constructs that guarantee a
    // return on every path. Loops are deliberately not assumed to execute.
    static bool blockAlwaysReturns(const Block& block) {
        for (const auto& statement : block.statements) {
            if (dynamic_cast<const ReturnStmt*>(statement.get()))
                return true;

            if (auto* nested = dynamic_cast<const Block*>(statement.get())) {
                if (blockAlwaysReturns(*nested))
                    return true;
            }

            if (auto* branch = dynamic_cast<const IfStmt*>(statement.get())) {
                if (branch->elseBlock && blockAlwaysReturns(*branch->thenBlock) &&
                    blockAlwaysReturns(*branch->elseBlock)) {
                    return true;
                }
            }

            if (auto* match = dynamic_cast<const MatchStmt*>(statement.get())) {
                bool hasWildcard = false;
                bool everyArmReturns = !match->arms.empty();
                for (const auto& arm : match->arms) {
                    hasWildcard = hasWildcard || arm.wildcard;
                    if (!arm.body || !blockAlwaysReturns(*arm.body)) {
                        everyArmReturns = false;
                    }
                }
                if (hasWildcard && everyArmReturns)
                    return true;
            }
        }
        return false;
    }

    void SemanticAnalyzer::analyze(Program& p) {

        diagnostics_.clear();
        funcs_.clear();
        structs_.clear();
        enums_.clear();

        const auto located = [](const std::string& message, const SourceSpan& span) {
            if (!span.valid || message.rfind(" at ") != std::string::npos)
                return message;
            return message + " at " + std::to_string(span.start.line) + ":" +
                   std::to_string(span.start.column);
        };
        const auto report = [&](const std::string& message, const SourceSpan& span) {
            diagnostics_.push_back(located(message, span));
        };

        for (auto& s : p.structs) {
            if (structs_.contains(s.name) || enums_.contains(s.name)) {
                report("duplicate type: " + s.name, s.span);
            } else {
                structs_[s.name] = &s;
            }

            std::unordered_map<std::string, bool> fieldNames;
            for (const auto& field : s.fields) {
                if (fieldNames.contains(field.name)) {
                    report("duplicate field '" + field.name + "' in struct '" + s.name + "'",
                           field.span.valid ? field.span : s.span);
                } else {
                    fieldNames[field.name] = true;
                }
            }
        }
        for (auto& e : p.enums) {
            if (structs_.contains(e.name) || enums_.contains(e.name)) {
                report("duplicate type: " + e.name, e.span);
            } else {
                enums_[e.name] = &e;
            }
            std::unordered_map<std::string, bool> variantNames;
            for (const auto& variant : e.variants) {
                if (variantNames.contains(variant))
                    report("duplicate enum variant '" + variant + "' in enum '" + e.name + "'",
                           e.span);
                else
                    variantNames[variant] = true;
            }
        }
        for (auto& f : p.functions) {
            if (funcs_.contains(f.name)) {
                report("duplicate function: " + f.name, f.span);
                continue;
            }
            if (builtinSignature(f.name).has_value()) {
                report("function name is reserved by builtin runtime: " + f.name, f.span);
                continue;
            }
            funcs_[f.name] = &f;
        }
        std::function<void(const Type&)> validateType;

        validateType = [&](const Type& t) {
            if (t.kind == TypeKind::Struct && !structs_.contains(t.name))
                throw std::runtime_error("unknown type: " + t.name);

            if (t.kind == TypeKind::Enum && !enums_.contains(t.name))
                throw std::runtime_error("unknown enum: " + t.name);

            if (t.kind == TypeKind::Pointer && t.pointee) {
                validateType(*t.pointee);
            }
            if (t.kind == TypeKind::Array && t.element) {
                validateType(*t.element);
            }
        };

        const auto validateTypeAt = [&](const Type& type, const SourceSpan& span) {
            try {
                validateType(type);
            } catch (const std::exception& error) {
                report(error.what(), span);
            }
        };

        for (auto& s : p.structs)
            for (auto& field : s.fields)
                validateTypeAt(field.type, field.span.valid ? field.span : s.span);

        for (auto& f : p.functions) {
            for (auto& a : f.params)
                validateTypeAt(a.type, a.span.valid ? a.span : f.span);
            validateTypeAt(f.ret, f.span);
            if (f.external)
                continue;
            std::unordered_map<std::string, Symbol> env;
            for (auto& a : f.params) {
                if (env.contains(a.name)) {
                    report("duplicate parameter: " + a.name, a.span.valid ? a.span : f.span);
                    continue;
                }
                env[a.name] = {a.type, true};
            }
            if (f.body) {
                block(*f.body, env, f.ret, false);
                if (f.ret.kind != TypeKind::Void && !blockAlwaysReturns(*f.body)) {
                    std::string message =
                        "function may exit without returning a value: " + f.name;
                    if (f.span.valid)
                        message += " at " + std::to_string(f.span.start.line) + ":" +
                                   std::to_string(f.span.start.column);
                    diagnostics_.push_back(std::move(message));
                }
            }
        }

        if (!diagnostics_.empty()) {
            std::ostringstream summary;
            summary << "NEXUS found " << diagnostics_.size() << " semantic error"
                    << (diagnostics_.size() == 1 ? "" : "s") << ":\n";
            for (const auto& diagnostic : diagnostics_)
                summary << "  - " << diagnostic << "\n";
            throw std::runtime_error(summary.str());
        }
    }
    Type SemanticAnalyzer::expr(Expr* e, std::unordered_map<std::string, Symbol>& env) {
        try {
            return exprImpl(e, env);
        } catch (const std::exception& error) {
            const std::string message = error.what();
            if (e && e->span.valid && message.rfind(" at ") == std::string::npos) {
                throw std::runtime_error(message + " at " +
                                         std::to_string(e->span.start.line) + ":" +
                                         std::to_string(e->span.start.column));
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
            auto& en = getEnum(ev->enumName);
            bool ok = false;
            for (auto& v : en.variants)
                if (v == ev->variant) {
                    ok = true;
                    break;
                }
            if (!ok)
                throw std::runtime_error("unknown enum variant: " + ev->enumName +
                                         "::" + ev->variant);
            return Type::enumerated(ev->enumName);
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
            if (auto* c = dynamic_cast<IntExpr*>(ix->index.get())) {
                if (c->value < 0 || static_cast<size_t>(c->value) >= ot.arraySize)
                    throw std::runtime_error("array index is out of bounds");
            }
            return *ot.element;
        }
        if (auto* u = dynamic_cast<UnaryExpr*>(e)) {
            auto t = expr(u->rhs.get(), env);
            if (u->op == TokenKind::Minus && (t.kind != TypeKind::I64 && t.kind != TypeKind::F64))
                throw std::runtime_error("unary '-' requires number");
            if (u->op == TokenKind::Bang && t.kind != TypeKind::Bool)
                throw std::runtime_error("'!' requires bool");
            if (u->op == TokenKind::Ampersand) {
                if (auto* v = dynamic_cast<VarExpr*>(u->rhs.get())) {
                    auto it = env.find(v->name);
                    if (it == env.end())
                        throw std::runtime_error("unknown variable: " + v->name);
                } else if (!dynamic_cast<MemberExpr*>(u->rhs.get()) &&
                           !dynamic_cast<IndexExpr*>(u->rhs.get()))
                    throw std::runtime_error("'&' requires a variable, field, or array element");
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
    void SemanticAnalyzer::block(Block& b,
                                 std::unordered_map<std::string, Symbol>& env,
                                 Type ret,
                                 bool inLoop) {

        for (auto& s : b.statements) {
            try {
            if (auto* nested = dynamic_cast<Block*>(s.get())) {
                auto localEnv = env;
                block(*nested, localEnv, ret, inLoop);
            } else if (auto* l = dynamic_cast<LetStmt*>(s.get())) {
                if (env.contains(l->name))
                    throw std::runtime_error("duplicate variable: " + l->name);
                auto t = expr(l->init.get(), env);
                if (l->type.kind != TypeKind::Unknown && !sameType(l->type, t))
                    throw std::runtime_error("cannot initialize '" + l->name + "': expected '" +
                                             describeType(l->type) + "' but got '" +
                                             describeType(t) + "'");
                l->type = t;
                env[l->name] = {t, l->mut};
            } else if (auto* a = dynamic_cast<AssignStmt*>(s.get())) {
                auto it = env.find(a->name);
                if (it == env.end())
                    throw std::runtime_error("unknown variable: " + a->name);
                if (!it->second.mut)
                    throw std::runtime_error("cannot assign to immutable variable: " + a->name);
                auto actual = expr(a->value.get(), env);
                if (!sameType(actual, it->second.type))
                    throw std::runtime_error("cannot assign to '" + a->name + "': expected '" +
                                             describeType(it->second.type) + "' but got '" +
                                             describeType(actual) + "'");
            } else if (auto* ma = dynamic_cast<MemberAssignStmt*>(s.get())) {
                auto t = expr(ma->object.get(), env);
                if (t.kind != TypeKind::Struct)
                    throw std::runtime_error("member assignment requires struct");
                if (auto* ov = dynamic_cast<VarExpr*>(ma->object.get())) {
                    auto it = env.find(ov->name);
                    if (it != env.end() && !it->second.mut)
                        throw std::runtime_error(
                            "cannot modify field through immutable variable: " + ov->name);
                }
                auto& st = getStruct(t.name);
                auto* f = fieldOf(st, ma->member);
                if (!f)
                    throw std::runtime_error("unknown field: " + ma->member);
                auto actual = expr(ma->value.get(), env);
                if (!sameType(actual, f->type))
                    throw std::runtime_error("cannot assign field '" + ma->member +
                                             "': expected '" + describeType(f->type) +
                                             "' but got '" + describeType(actual) + "'");
            } else if (auto* ia = dynamic_cast<IndexAssignStmt*>(s.get())) {
                auto ot = expr(ia->object.get(), env);
                if (auto* ov = dynamic_cast<VarExpr*>(ia->object.get())) {
                    auto it = env.find(ov->name);
                    if (it != env.end() && !it->second.mut)
                        throw std::runtime_error(
                            "cannot modify array through immutable variable: " + ov->name);
                }
                if (ot.kind != TypeKind::Array || !ot.element)
                    throw std::runtime_error("index assignment requires an array");
                if (expr(ia->index.get(), env).kind != TypeKind::I64)
                    throw std::runtime_error("array index must be i64");
                if (auto* c = dynamic_cast<IntExpr*>(ia->index.get()))
                    if (c->value < 0 || static_cast<size_t>(c->value) >= ot.arraySize)
                        throw std::runtime_error("array index is out of bounds");
                auto actual = expr(ia->value.get(), env);
                if (!sameType(actual, *ot.element))
                    throw std::runtime_error("array element assignment expects '" +
                                             describeType(*ot.element) + "' but got '" +
                                             describeType(actual) + "'");
            } else if (auto* e = dynamic_cast<ExprStmt*>(s.get()))
                expr(e->expr.get(), env);

            else if (auto* r = dynamic_cast<ReturnStmt*>(s.get())) {
                auto t = r->expr ? expr(r->expr.get(), env) : Type::void_();
                if (!sameType(t, ret))
                    throw std::runtime_error("return expects '" + describeType(ret) +
                                             "' but got '" + describeType(t) + "'");
            } else if (auto* ifs = dynamic_cast<IfStmt*>(s.get())) {
                {
                    auto actual = expr(ifs->cond.get(), env);
                    if (actual.kind != TypeKind::Bool)
                        throw std::runtime_error("if condition expects 'bool' but got '" +
                                                 describeType(actual) + "'");
                }
                auto a = env;
                block(*ifs->thenBlock, a, ret, inLoop);
                if (ifs->elseBlock) {
                    auto c = env;
                    block(*ifs->elseBlock, c, ret, inLoop);
                }
            } else if (auto* w = dynamic_cast<WhileStmt*>(s.get())) {
                {
                    auto actual = expr(w->cond.get(), env);
                    if (actual.kind != TypeKind::Bool)
                        throw std::runtime_error("while condition expects 'bool' but got '" +
                                                 describeType(actual) + "'");
                }
                auto a = env;
                block(*w->body, a, ret, true);
            } else if (auto* mm = dynamic_cast<MatchStmt*>(s.get())) {

                auto vt = expr(mm->value.get(), env);
                if (vt.kind != TypeKind::I64 && vt.kind != TypeKind::Bool &&
                    vt.kind != TypeKind::Enum)
                    throw std::runtime_error("match supports i64, bool, or enum values");
                bool wildcard = false;
                for (auto& arm : mm->arms) {
                    if (arm.wildcard) {
                        if (wildcard)
                            throw std::runtime_error("duplicate wildcard match arm");
                        wildcard = true;
                    } else {
                        auto pt = expr(arm.pattern.get(), env);
                        if (!sameType(pt, vt))
                            throw std::runtime_error("match pattern type mismatch");
                    }
                    auto local = env;
                    block(*arm.body, local, ret, inLoop);
                }
            } else if (dynamic_cast<BreakStmt*>(s.get()) || dynamic_cast<ContinueStmt*>(s.get())) {
                if (!inLoop)
                    throw std::runtime_error("break/continue used outside loop");
            }
            } catch (const std::exception& error) {
                std::string diagnostic = error.what();
                if (diagnostic.rfind(" at ") == std::string::npos && s && s->span.valid)
                    diagnostic += " at " + std::to_string(s->span.start.line) + ":" +
                                  std::to_string(s->span.start.column);
                diagnostics_.push_back(std::move(diagnostic));
            }
        }
    }
} // namespace nexus
