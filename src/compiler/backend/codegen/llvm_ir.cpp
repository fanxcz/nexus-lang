#include "codegen/llvm_ir.hpp"

#include "builtins.hpp"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
namespace nexus {

    class CG {

    public:
        explicit CG(CodegenOptions o) : options(std::move(o)) {}
        CodegenOptions options;
        std::ostringstream module, functionsIR, body;
        int reg = 0, label = 0;
        bool terminated = false;

        std::unordered_map<std::string, std::string> vars;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, Function*> functions;
        std::unordered_map<std::string, StructDecl*> structs;
        std::unordered_map<std::string, EnumDecl*> enums;

        std::vector<std::string> globals;
        Type currentReturnType = Type::void_();
        std::vector<std::string> breakLabels, continueLabels;
        // Each entry represents one lexical scope; calls are stored as fully typed IR
        // instructions after their arguments have been evaluated at the defer site.
        std::vector<std::vector<std::string>> deferredCalls;
        std::vector<std::size_t> loopDeferScopeDepth;

        std::string next() {
            return "%" + std::to_string(++reg);
        }
        std::string lab(const std::string& p) {
            return p + std::to_string(++label);
        }
        void emit(const std::string& s) {
            body << s << '\n';
        }
        void emitModule(const std::string& s) {
            module << s << '\n';
        }
        static std::string llvmType(const Type& t) {
            switch (t.kind) {
            case TypeKind::I64:
                return "i64";
            case TypeKind::F64:
                return "double";
            case TypeKind::Bool:
                return "i1";
            case TypeKind::String:
                return "ptr";
            case TypeKind::Void:
                return "void";
            case TypeKind::Pointer:
                return "ptr";
            case TypeKind::Struct:
                return "%struct." + t.name;
            case TypeKind::Enum:
                return "{ i64, i64 }";
            case TypeKind::Array:
                return "[" + std::to_string(t.arraySize) + " x " + llvmType(*t.element) + "]";
            default:
                return "i64";
            }
        }
        static std::string escape(const std::string& s) {
            std::ostringstream q;
            for (unsigned char c : s) {
                switch (c) {
                case '\\':
                    q << "\\5C";
                    break;
                case '"':
                    q << "\\22";
                    break;
                case '\n':
                    q << "\\0A";
                    break;
                case '\r':
                    q << "\\0D";
                    break;
                case '\t':
                    q << "\\09";
                    break;
                default:
                    if (c < 32) {
                        char b[5];
                        std::snprintf(b, sizeof b, "\\%02X", c);
                        q << b;
                    } else
                        q << c;
                }
            }
            return q.str();
        }
        const StructDecl& getStruct(const std::string& n) const {
            auto it = structs.find(n);
            if (it == structs.end())
                throw std::runtime_error("unknown struct in codegen: " + n);
            return *it->second;
        }
        const EnumDecl& getEnum(const std::string& n) const {
            auto it = enums.find(n);
            if (it == enums.end())
                throw std::runtime_error("unknown enum in codegen: " + n);
            return *it->second;
        }
        std::size_t fieldIndex(const StructDecl& s, const std::string& n) const {
            for (std::size_t i = 0; i < s.fields.size(); ++i)
                if (s.fields[i].name == n)
                    return i;
            throw std::runtime_error("unknown field in codegen: " + n);
        }
        long long enumTag(const EnumVariantExpr& e) const {
            const auto& en = getEnum(e.enumName);
            for (std::size_t i = 0; i < en.variants.size(); ++i)
                if (en.variants[i].name == e.variant)
                    return static_cast<long long>(i);
            throw std::runtime_error("unknown enum variant in codegen");
        }
        Type typeOf(const Expr* e) {

            if (dynamic_cast<const IntExpr*>(e))
                return Type::i64();
            if (dynamic_cast<const FloatExpr*>(e))
                return Type::f64();
            if (dynamic_cast<const BoolExpr*>(e))
                return Type::boolean();
            if (dynamic_cast<const StringExpr*>(e))
                return Type::string();

            if (auto* v = dynamic_cast<const VarExpr*>(e))
                return types.at(v->name);

            if (auto* ev = dynamic_cast<const EnumVariantExpr*>(e))
                return Type::enumerated(ev->enumName);

            if (auto* a = dynamic_cast<const ArrayExpr*>(e)) {
                if (a->elements.empty())
                    throw std::runtime_error("empty array");
                return Type::array(typeOf(a->elements[0].get()), a->elements.size());
            }
            if (auto* ix = dynamic_cast<const IndexExpr*>(e)) {
                auto t = typeOf(ix->object.get());
                if (t.kind != TypeKind::Array || !t.element)
                    throw std::runtime_error("index on non-array");
                return *t.element;
            }
            if (auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                auto t = typeOf(u->rhs.get());
                if (u->op == TokenKind::Ampersand)
                    return Type::ptr(t);
                if (u->op == TokenKind::Star)
                    return *t.pointee;
                return t;
            }
            if (auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                auto l = typeOf(b->lhs.get());
                switch (b->op) {
                case TokenKind::EqualEqual:
                case TokenKind::BangEqual:
                case TokenKind::Less:
                case TokenKind::LessEqual:
                case TokenKind::Greater:
                case TokenKind::GreaterEqual:
                case TokenKind::AndAnd:
                case TokenKind::OrOr:
                    return Type::boolean();
                default:
                    return l;
                }
            }
            if (auto* c = dynamic_cast<const CallExpr*>(e)) {
                if (c->callee == "print")
                    return Type::void_();
                if (auto bi = builtinSignature(c->callee))
                    return bi->ret;
                return functions.at(c->callee)->ret;
            }
            if (auto* m = dynamic_cast<const MemberExpr*>(e)) {
                auto t = typeOf(m->object.get());
                return getStruct(t.name).fields[fieldIndex(getStruct(t.name), m->member)].type;
            }
            if (auto* s = dynamic_cast<const StructInitExpr*>(e))
                return Type::named(s->typeName);

            throw std::runtime_error("unknown expression type in codegen");
        }
        std::string stringValue(const std::string& s) {
            std::string g = "@.str" + std::to_string(globals.size());
            size_t n = s.size() + 1;
            std::ostringstream x;
            x << g << " = private unnamed_addr constant [" << n << " x i8] c\"" << escape(s)
              << "\\00\", align 1";
            globals.push_back(x.str());
            std::string p = next();
            emit(p + " = getelementptr inbounds [" + std::to_string(n) + " x i8], ptr " + g +
                 ", i64 0, i64 0");
            return p;
        }
        std::string loadVar(const std::string& name) {
            auto r = next();
            auto t = types.at(name);
            emit(r + " = load " + llvmType(t) + ", ptr " + vars.at(name));
            return r;
        }
        std::string addressOf(const Expr* e) {

            if (auto* v = dynamic_cast<const VarExpr*>(e))
                return vars.at(v->name);

            if (auto* m = dynamic_cast<const MemberExpr*>(e)) {
                auto ot = typeOf(m->object.get());
                auto base = addressOf(m->object.get());
                auto idx = fieldIndex(getStruct(ot.name), m->member);
                auto r = next();
                emit(r + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                     ", i32 0, i32 " + std::to_string(idx));
                return r;
            }
            if (auto* ix = dynamic_cast<const IndexExpr*>(e)) {
                auto ot = typeOf(ix->object.get());
                if (ot.kind != TypeKind::Array || !ot.element)
                    throw std::runtime_error("index address requires array");
                auto base = addressOf(ix->object.get());
                auto idx = expr(ix->index.get());
                emit("call void @nexus_bounds_check(i64 " + idx + ", i64 " +
                     std::to_string(ot.arraySize) + ")");
                auto r = next();
                emit(r + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                     ", i64 0, i64 " + idx);
                return r;
            }
            throw std::runtime_error("address-of requires a variable, field, or array element");
        }
        std::string memberLoad(const MemberExpr* m) {
            auto addr = addressOf(m);
            auto t = typeOf(m);
            auto r = next();
            emit(r + " = load " + llvmType(t) + ", ptr " + addr);
            return r;
        }
        std::string expr(const Expr* e) {

            if (auto* x = dynamic_cast<const IntExpr*>(e))
                return std::to_string(x->value);

            if (auto* x = dynamic_cast<const FloatExpr*>(e)) {
                std::ostringstream o;
                o.setf(std::ios::scientific);
                o << std::setprecision(17) << x->value;
                return o.str();
            }
            if (auto* x = dynamic_cast<const BoolExpr*>(e))
                return x->value ? "true" : "false";

            if (auto* x = dynamic_cast<const StringExpr*>(e))
                return stringValue(x->value);

            if (auto* t = dynamic_cast<const TryExpr*>(e)) {
                const auto inner = expr(t->expr.get());
                const auto tag = next();
                emit(tag + " = extractvalue { i64, i64 } " + inner + ", 0");
                const auto cmp = next();
                emit(cmp + " = icmp ne i64 " + tag + ", 0");
                const auto errLabel = lab("try.err");
                const auto okLabel = lab("try.ok");
                emit("br i1 " + cmp + ", label %" + errLabel + ", label %" + okLabel);
                emit(errLabel + ":");
                if (currentReturnType.kind == TypeKind::Void)
                    emit("ret void");
                else
                    emit("ret " + llvmType(currentReturnType) + " " + inner);
                terminated = true;
                emit(okLabel + ":");
                terminated = false;
                const auto payload = next();
                emit(payload + " = extractvalue { i64, i64 } " + inner + ", 1");
                return payload;
            }

            if (auto* x = dynamic_cast<const EnumVariantExpr*>(e)) {
                const auto tag = enumTag(*x);
                const auto& en = getEnum(x->enumName);
                const EnumVariant* variant = nullptr;
                for (const auto& v : en.variants)
                    if (v.name == x->variant) { variant = &v; break; }
                std::string payloadI64 = "0";
                if (variant && variant->payload && x->payload) {
                    const auto payloadType = *variant->payload;
                    const auto val = expr(x->payload.get());
                    if (payloadType.kind == TypeKind::I64)
                        payloadI64 = val;
                    else if (payloadType.kind == TypeKind::Bool)
                        payloadI64 = val; // i1, need zext
                    else if (payloadType.kind == TypeKind::String || payloadType.kind == TypeKind::Pointer) {
                        auto c = next();
                        emit(c + " = ptrtoint ptr " + val + " to i64");
                        payloadI64 = c;
                    } else if (payloadType.kind == TypeKind::F64) {
                        auto c = next();
                        emit(c + " = bitcast double " + val + " to i64");
                        payloadI64 = c;
                    }
                }
                auto r0 = next();
                emit(r0 + " = insertvalue { i64, i64 } undef, i64 " + std::to_string(tag) + ", 0");
                auto r1 = next();
                emit(r1 + " = insertvalue { i64, i64 } " + r0 + ", i64 " + payloadI64 + ", 1");
                return r1;
            }

            if (auto* x = dynamic_cast<const VarExpr*>(e))
                return loadVar(x->name);

            if (auto* ix = dynamic_cast<const IndexExpr*>(e)) {
                auto t = typeOf(ix);
                auto addr = addressOf(ix);
                auto r = next();
                emit(r + " = load " + llvmType(t) + ", ptr " + addr);
                return r;
            }
            if (auto* s = dynamic_cast<const ArrayExpr*>(e)) {
                Type at = typeOf(e);
                std::string cur = "undef";
                for (std::size_t i = 0; i < s->elements.size(); ++i) {
                    auto v = expr(s->elements[i].get());
                    auto r = next();
                    emit(r + " = insertvalue " + llvmType(at) + " " + cur + ", " +
                         llvmType(*at.element) + " " + v + ", " + std::to_string(i));
                    cur = r;
                }
                return cur;
            }
            if (auto* m = dynamic_cast<const MemberExpr*>(e))
                return memberLoad(m);

            if (auto* s = dynamic_cast<const StructInitExpr*>(e)) {
                const auto& st = getStruct(s->typeName);
                std::string cur = "undef";
                Type stt = Type::named(s->typeName);
                for (std::size_t i = 0; i < st.fields.size(); ++i) {
                    auto it = std::find_if(s->fields.begin(), s->fields.end(), [&](const auto& x) {
                        return x.first == st.fields[i].name;
                    });
                    if (it == s->fields.end())
                        throw std::runtime_error("missing struct field");
                    auto v = expr(it->second.get());
                    auto r = next();
                    emit(r + " = insertvalue " + llvmType(stt) + " " + cur + ", " +
                         llvmType(st.fields[i].type) + " " + v + ", " + std::to_string(i));
                    cur = r;
                }
                return cur;
            }
            if (auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                auto a = expr(u->rhs.get());
                auto t = typeOf(u->rhs.get());
                if (u->op == TokenKind::Ampersand)
                    return addressOf(u->rhs.get());
                auto r = next();
                if (u->op == TokenKind::Minus) {
                    if (t.kind == TypeKind::I64)
                        emit(r + " = sub i64 0, " + a);
                    else
                        emit(r + " = fneg double " + a);
                    return r;
                }
                if (u->op == TokenKind::Bang) {
                    emit(r + " = xor i1 " + a + ", true");
                    return r;
                }
                if (u->op == TokenKind::Star) {
                    emit(r + " = load " + llvmType(*t.pointee) + ", ptr " + a);
                    return r;
                }
            }
            if (auto* b = dynamic_cast<const BinaryExpr*>(e)) {

                auto l = expr(b->lhs.get()), rv = expr(b->rhs.get());
                auto t = typeOf(b->lhs.get());

                if (t.kind == TypeKind::String) {
                    if (b->op == TokenKind::Plus) {
                        auto r = next();
                        emit(r + " = call ptr @nexus_str_concat(ptr " + l + ", ptr " + rv + ")");
                        return r;
                    }
                    if (b->op == TokenKind::EqualEqual || b->op == TokenKind::BangEqual) {
                        auto eq = next();
                        auto r = next();
                        emit(eq + " = call i1 @nexus_str_equal(ptr " + l + ", ptr " + rv + ")");
                        if (b->op == TokenKind::BangEqual)
                            emit(r + " = xor i1 " + eq + ", true");
                        else
                            emit(r + " = or i1 " + eq + ", false");
                        return r;
                    }
                    throw std::runtime_error("unsupported string operator");
                }
                auto r = next();

                if (t.kind == TypeKind::F64) {
                    switch (b->op) {
                    case TokenKind::Plus:
                        emit(r + " = fadd double " + l + ", " + rv);
                        break;
                    case TokenKind::Minus:
                        emit(r + " = fsub double " + l + ", " + rv);
                        break;
                    case TokenKind::Star:
                        emit(r + " = fmul double " + l + ", " + rv);
                        break;
                    case TokenKind::Slash:
                        emit(r + " = fdiv double " + l + ", " + rv);
                        break;
                    case TokenKind::EqualEqual:
                        emit(r + " = fcmp oeq double " + l + ", " + rv);
                        break;
                    case TokenKind::BangEqual:
                        emit(r + " = fcmp one double " + l + ", " + rv);
                        break;
                    case TokenKind::Less:
                        emit(r + " = fcmp olt double " + l + ", " + rv);
                        break;
                    case TokenKind::LessEqual:
                        emit(r + " = fcmp ole double " + l + ", " + rv);
                        break;
                    case TokenKind::Greater:
                        emit(r + " = fcmp ogt double " + l + ", " + rv);
                        break;
                    case TokenKind::GreaterEqual:
                        emit(r + " = fcmp oge double " + l + ", " + rv);
                        break;
                    default:
                        throw std::runtime_error("unsupported float operator");
                    }
                    return r;
                }
                if (t.kind == TypeKind::Bool) {
                    if (b->op == TokenKind::AndAnd)
                        emit(r + " = and i1 " + l + ", " + rv);
                    else if (b->op == TokenKind::OrOr)
                        emit(r + " = or i1 " + l + ", " + rv);
                    else if (b->op == TokenKind::EqualEqual)
                        emit(r + " = icmp eq i1 " + l + ", " + rv);
                    else if (b->op == TokenKind::BangEqual)
                        emit(r + " = icmp ne i1 " + l + ", " + rv);
                    else
                        throw std::runtime_error("unsupported bool operator");
                    return r;
                }
                switch (b->op) {
                case TokenKind::Plus:
                    emit(r + " = add i64 " + l + ", " + rv);
                    break;
                case TokenKind::Minus:
                    emit(r + " = sub i64 " + l + ", " + rv);
                    break;
                case TokenKind::Star:
                    emit(r + " = mul i64 " + l + ", " + rv);
                    break;
                case TokenKind::Slash:
                    emit(r + " = sdiv i64 " + l + ", " + rv);
                    break;
                case TokenKind::Percent:
                    emit(r + " = srem i64 " + l + ", " + rv);
                    break;
                case TokenKind::EqualEqual:
                    emit(r + " = icmp eq i64 " + l + ", " + rv);
                    break;
                case TokenKind::BangEqual:
                    emit(r + " = icmp ne i64 " + l + ", " + rv);
                    break;
                case TokenKind::Less:
                    emit(r + " = icmp slt i64 " + l + ", " + rv);
                    break;
                case TokenKind::LessEqual:
                    emit(r + " = icmp sle i64 " + l + ", " + rv);
                    break;
                case TokenKind::Greater:
                    emit(r + " = icmp sgt i64 " + l + ", " + rv);
                    break;
                case TokenKind::GreaterEqual:
                    emit(r + " = icmp sge i64 " + l + ", " + rv);
                    break;
                default:
                    throw std::runtime_error("unsupported integer operator");
                }
                return r;
            }
            if (auto* c = dynamic_cast<const CallExpr*>(e)) {
                auto it = functions.find(c->callee);
                if (c->callee == "print") {
                    auto av = expr(c->args[0].get());
                    auto t = typeOf(c->args[0].get());
                    if (t.kind == TypeKind::I64)
                        emit("call void @nexus_print_i64(i64 " + av + ")");
                    else if (t.kind == TypeKind::F64)
                        emit("call void @nexus_print_f64(double " + av + ")");
                    else if (t.kind == TypeKind::Bool)
                        emit("call void @nexus_print_bool(i1 " + av + ")");
                    else if (t.kind == TypeKind::String)
                        emit("call void @nexus_print_str(ptr " + av + ")");
                    return "0";
                }
                if (auto bi = builtinSignature(c->callee)) {
                    std::ostringstream call;
                    call << "call " << llvmType(bi->ret) << " @" << bi->runtimeName << "(";
                    for (size_t i = 0; i < c->args.size(); ++i) {
                        if (i)
                            call << ", ";
                        auto av = expr(c->args[i].get());
                        call << llvmType(bi->params[i]) << " " << av;
                    }
                    call << ")";
                    if (bi->ret.kind == TypeKind::Void) {
                        emit(call.str());
                        return "0";
                    }
                    auto r = next();
                    emit(r + " = " + call.str());
                    return r;
                }
                if (it == functions.end())
                    throw std::runtime_error("unknown function in codegen: " + c->callee);
                auto* fn = it->second;
                std::ostringstream call;
                call << "call " + llvmType(fn->ret) + " @" << fn->name << "(";
                for (size_t i = 0; i < c->args.size(); ++i) {
                    if (i)
                        call << ", ";
                    auto av = expr(c->args[i].get());
                    call << llvmType(fn->params[i].type) << " " << av;
                }
                call << ")";
                if (fn->ret.kind == TypeKind::Void) {
                    emit(call.str());
                    return "0";
                }
                auto r = next();
                emit(r + " = " + call.str());
                return r;
            }
            throw std::runtime_error("unsupported expression in codegen");
        }
        std::string deferredCall(const CallExpr* c) {
            if (!c)
                throw std::runtime_error("defer requires a function call");

            if (c->callee == "print") {
                if (c->args.size() != 1)
                    throw std::runtime_error("defer print expects one argument");
                const auto value = expr(c->args[0].get());
                const auto type = typeOf(c->args[0].get());
                if (type.kind == TypeKind::I64)
                    return "call void @nexus_print_i64(i64 " + value + ")";
                if (type.kind == TypeKind::F64)
                    return "call void @nexus_print_f64(double " + value + ")";
                if (type.kind == TypeKind::Bool)
                    return "call void @nexus_print_bool(i1 " + value + ")";
                if (type.kind == TypeKind::String)
                    return "call void @nexus_print_str(ptr " + value + ")";
                throw std::runtime_error("defer print: unsupported type");
            }

            std::ostringstream call;
            if (auto bi = builtinSignature(c->callee)) {
                if (bi->ret.kind != TypeKind::Void)
                    throw std::runtime_error("defer requires a call returning void");
                call << "call void @" << bi->runtimeName << "(";
                for (std::size_t i = 0; i < c->args.size(); ++i) {
                    if (i)
                        call << ", ";
                    const auto value = expr(c->args[i].get());
                    call << llvmType(bi->params[i]) << " " << value;
                }
            } else {
                const auto it = functions.find(c->callee);
                if (it == functions.end())
                    throw std::runtime_error("unknown function in defer: " + c->callee);
                const auto* fn = it->second;
                if (fn->ret.kind != TypeKind::Void)
                    throw std::runtime_error("defer requires a call returning void");
                call << "call void @" << fn->name << "(";
                for (std::size_t i = 0; i < c->args.size(); ++i) {
                    if (i)
                        call << ", ";
                    const auto value = expr(c->args[i].get());
                    call << llvmType(fn->params[i].type) << " " << value;
                }
            }
            call << ")";
            return call.str();
        }

        // Emit pending calls in reverse registration order. A depth of zero means
        // all active scopes; a loop records its entry depth so break/continue only
        // clean up scopes they actually leave.
        void emitDefersFrom(std::size_t keepDepth) {
            for (std::size_t depth = deferredCalls.size(); depth > keepDepth; --depth) {
                const auto& calls = deferredCalls[depth - 1];
                for (auto it = calls.rbegin(); it != calls.rend(); ++it)
                    emit(*it);
            }
        }

        bool stmtBlock(const Block* nested, Type ret) {
            auto savedVars = vars;
            auto savedTypes = types;
            deferredCalls.emplace_back();
            for (const auto& x : nested->statements) {
                stmt(x.get(), ret);
                if (terminated)
                    break;
            }
            const bool blockTerminated = terminated;
            if (!blockTerminated)
                emitDefersFrom(deferredCalls.size() - 1);
            deferredCalls.pop_back();
            vars = std::move(savedVars);
            types = std::move(savedTypes);
            return blockTerminated;
        }

        bool stmt(const Stmt* s, Type ret) {

            if (terminated)
                return true;

            if (auto* nested = dynamic_cast<const Block*>(s))
                return stmtBlock(nested, ret);
            if (auto* l = dynamic_cast<const LetStmt*>(s)) {
                auto ptr = next();
                vars[l->name] = ptr;
                types[l->name] = l->type;
                emit(ptr + " = alloca " + llvmType(l->type));
                auto v = expr(l->init.get());
                emit("store " + llvmType(l->type) + " " + v + ", ptr " + ptr);
                return false;
            }
            if (auto* a = dynamic_cast<const AssignStmt*>(s)) {
                auto v = expr(a->value.get());
                emit("store " + llvmType(types.at(a->name)) + " " + v + ", ptr " +
                     vars.at(a->name));
                return false;
            }
            if (auto* a = dynamic_cast<const MemberAssignStmt*>(s)) {
                auto ot = typeOf(a->object.get());
                auto base = addressOf(a->object.get());
                const auto& st = getStruct(ot.name);
                auto idx = fieldIndex(st, a->member);
                auto ftype = st.fields[idx].type;
                auto addr = next();
                emit(addr + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                     ", i32 0, i32 " + std::to_string(idx));
                auto v = expr(a->value.get());
                emit("store " + llvmType(ftype) + " " + v + ", ptr " + addr);
                return false;
            }
            if (auto* a = dynamic_cast<const IndexAssignStmt*>(s)) {
                Type ot = typeOf(a->object.get());
                if (ot.kind != TypeKind::Array || !ot.element)
                    throw std::runtime_error("index assignment requires array");
                auto idx = expr(a->index.get());
                emit("call void @nexus_bounds_check(i64 " + idx + ", i64 " +
                     std::to_string(ot.arraySize) + ")");
                auto base = addressOf(a->object.get());
                auto addr = next();
                emit(addr + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                     ", i64 0, i64 " + idx);
                auto v = expr(a->value.get());
                emit("store " + llvmType(*ot.element) + " " + v + ", ptr " + addr);
                return false;
            }
            if (dynamic_cast<const ExprStmt*>(s)) {
                expr(static_cast<const ExprStmt*>(s)->expr.get());
                return false;
            }
            if (auto* d = dynamic_cast<const DeferStmt*>(s)) {
                if (deferredCalls.empty())
                    throw std::runtime_error("defer used outside a lexical scope");
                const auto* call = dynamic_cast<const CallExpr*>(d->call.get());
                deferredCalls.back().push_back(deferredCall(call));
                return false;
            }
            if (auto* r = dynamic_cast<const ReturnStmt*>(s)) {
                // Evaluate the return value before running cleanup calls, but emit
                // the actual return only after every active scope has been cleaned.
                std::string value;
                if (r->expr)
                    value = expr(r->expr.get());
                emitDefersFrom(0);
                if (r->expr) {
                    emit("ret " + llvmType(ret) + " " + value);
                } else if (ret.kind == TypeKind::Void) {
                    emit("ret void");
                } else {
                    emit("ret " + llvmType(ret) + " 0");
                }
                terminated = true;
                return true;
            }
            if (auto* i = dynamic_cast<const IfStmt*>(s)) {
                auto c = expr(i->cond.get()), lt = lab("if.then"), lf = lab("if.else"),
                     le = lab("if.end");
                emit("br i1 " + c + ", label %" + lt + ", label %" + lf);
                emit(lt + ":");
                terminated = false;
                const bool tt = stmtBlock(i->thenBlock.get(), ret);
                if (!tt)
                    emit("br label %" + le);
                emit(lf + ":");
                terminated = false;
                bool et = false;
                if (i->elseBlock) {
                    et = stmtBlock(i->elseBlock.get(), ret);
                }
                if (!et)
                    emit("br label %" + le);
                if (tt && et) {
                    terminated = true;
                    return true;
                }
                emit(le + ":");
                terminated = false;
                return false;
            }
            if (auto* w = dynamic_cast<const WhileStmt*>(s)) {
                auto lc = lab("while.cond"), lb = lab("while.body"), le = lab("while.end");
                emit("br label %" + lc);
                emit(lc + ":");
                auto c = expr(w->cond.get());
                emit("br i1 " + c + ", label %" + lb + ", label %" + le);
                emit(lb + ":");
                terminated = false;
                breakLabels.push_back(le);
                continueLabels.push_back(lc);
                loopDeferScopeDepth.push_back(deferredCalls.size());
                stmtBlock(w->body.get(), ret);
                loopDeferScopeDepth.pop_back();
                breakLabels.pop_back();
                continueLabels.pop_back();
                if (!terminated)
                    emit("br label %" + lc);
                emit(le + ":");
                terminated = false;
                return false;
            }
            if (auto* f = dynamic_cast<const ForStmt*>(s)) {
                auto lc = lab("for.cond"), lb = lab("for.body"), li = lab("for.inc"),
                     le = lab("for.end");
                const auto savedVars = vars;
                const auto savedTypes = types;
                Type varType;

                if (f->rangeEnd) {
                    varType = Type::i64();
                    auto varPtr = next();
                    vars[f->name] = varPtr;
                    types[f->name] = varType;
                    emit(varPtr + " = alloca i64");
                    auto startValue = expr(f->start.get());
                    emit("store i64 " + startValue + ", ptr " + varPtr);

                    // The range end is evaluated once, before the loop starts.
                    auto endPtr = next();
                    emit(endPtr + " = alloca i64");
                    auto endValue = expr(f->rangeEnd.get());
                    emit("store i64 " + endValue + ", ptr " + endPtr);

                    emit("br label %" + lc);
                    emit(lc + ":");
                    auto current = next();
                    emit(current + " = load i64, ptr " + varPtr);
                    auto limit = next();
                    emit(limit + " = load i64, ptr " + endPtr);
                    auto inRange = next();
                    emit(inRange + " = icmp slt i64 " + current + ", " + limit);
                    emit("br i1 " + inRange + ", label %" + lb + ", label %" + le);
                } else {
                    auto collectionType = typeOf(f->collection.get());
                    const bool stringCollection = collectionType.kind == TypeKind::String;
                    varType = stringCollection ? Type::i64() : *collectionType.element;
                    auto collectionPtr = next();
                    emit(collectionPtr + " = alloca " + llvmType(collectionType));
                    auto collectionValue = expr(f->collection.get());
                    emit("store " + llvmType(collectionType) + " " + collectionValue + ", ptr " +
                         collectionPtr);

                    // Arrays use their compile-time length; strings use Unicode code-point count.
                    auto lengthPtr = next();
                    emit(lengthPtr + " = alloca i64");
                    if (stringCollection) {
                        auto length = next();
                        emit(length + " = call i64 @nexus_string_char_count(ptr " + collectionValue + ")");
                        emit("store i64 " + length + ", ptr " + lengthPtr);
                    } else {
                        emit("store i64 " + std::to_string(collectionType.arraySize) + ", ptr " +
                             lengthPtr);
                    }

                    auto indexPtr = next();
                    emit(indexPtr + " = alloca i64");
                    emit("store i64 0, ptr " + indexPtr);
                    auto varPtr = next();
                    vars[f->name] = varPtr;
                    types[f->name] = varType;
                    emit(varPtr + " = alloca " + llvmType(varType));

                    emit("br label %" + lc);
                    emit(lc + ":");
                    auto current = next();
                    emit(current + " = load i64, ptr " + indexPtr);
                    auto length = next();
                    emit(length + " = load i64, ptr " + lengthPtr);
                    auto inRange = next();
                    emit(inRange + " = icmp slt i64 " + current + ", " + length);
                    emit("br i1 " + inRange + ", label %" + lb + ", label %" + le);
                    emit(lb + ":");
                    if (stringCollection) {
                        auto element = next();
                        emit(element + " = call i64 @nexus_string_char_at(ptr " + collectionValue +
                             ", i64 " + current + ")");
                        emit("store i64 " + element + ", ptr " + varPtr);
                    } else {
                        auto elementPtr = next();
                        emit(elementPtr + " = getelementptr inbounds " + llvmType(collectionType) +
                             ", ptr " + collectionPtr + ", i64 0, i64 " + current);
                        auto element = next();
                        emit(element + " = load " + llvmType(varType) + ", ptr " + elementPtr);
                        emit("store " + llvmType(varType) + " " + element + ", ptr " + varPtr);
                    }
                    terminated = false;
                    breakLabels.push_back(le);
                    continueLabels.push_back(li);
                    loopDeferScopeDepth.push_back(deferredCalls.size());
                    stmtBlock(f->body.get(), ret);
                    loopDeferScopeDepth.pop_back();
                    breakLabels.pop_back();
                    continueLabels.pop_back();
                    if (!terminated)
                        emit("br label %" + li);
                    emit(li + ":");
                    terminated = false;
                    auto index = next();
                    emit(index + " = load i64, ptr " + indexPtr);
                    auto nextIndex = next();
                    emit(nextIndex + " = add i64 " + index + ", 1");
                    emit("store i64 " + nextIndex + ", ptr " + indexPtr);
                    emit("br label %" + lc);
                }

                if (f->rangeEnd) {
                    emit(lb + ":");
                    terminated = false;
                    breakLabels.push_back(le);
                    continueLabels.push_back(li);
                    loopDeferScopeDepth.push_back(deferredCalls.size());
                    stmtBlock(f->body.get(), ret);
                    loopDeferScopeDepth.pop_back();
                    breakLabels.pop_back();
                    continueLabels.pop_back();
                    if (!terminated)
                        emit("br label %" + li);
                    emit(li + ":");
                    terminated = false;
                    auto current = next();
                    emit(current + " = load i64, ptr " + vars[f->name]);
                    auto nextValue = next();
                    emit(nextValue + " = add i64 " + current + ", 1");
                    emit("store i64 " + nextValue + ", ptr " + vars[f->name]);
                    emit("br label %" + lc);
                }

                vars = savedVars;
                types = savedTypes;
                emit(le + ":");
                terminated = false;
                return false;
            }
            if (auto* mm = dynamic_cast<const MatchStmt*>(s)) {

                auto value = expr(mm->value.get());
                std::string valueTag = value;
                Type vt = typeOf(mm->value.get());
                if (vt.kind == TypeKind::Enum) {
                    auto t = next();
                    emit(t + " = extractvalue { i64, i64 } " + value + ", 0");
                    valueTag = t;
                }
                auto end = lab("match.end");
                std::vector<std::string> bodies, tests;
                for (std::size_t k = 0; k < mm->arms.size(); ++k) {
                    bodies.push_back(lab("match.body"));
                    tests.push_back(k + 1 < mm->arms.size() ? lab("match.test") : end);
                }
                std::string first = lab("match.test");
                emit("br label %" + first);
                std::string current = first;
                bool allTerminated = true;
                bool hasCatchAll = false;
                for (std::size_t k = 0; k < mm->arms.size(); ++k) {
                    emit(current + ":");
                    auto& arm = mm->arms[k];
                    const auto savedVars = vars;
                    const auto savedTypes = types;
                    if (!arm.binding.empty()) {
                        Type bindType = vt;
                        std::string bindValue = value;
                        if (vt.kind == TypeKind::Enum) {
                            const auto* evp = dynamic_cast<const EnumVariantExpr*>(arm.pattern.get());
                            if (evp) {
                                const auto& en = getEnum(evp->enumName);
                                for (const auto& v : en.variants) {
                                    if (v.name == evp->variant && v.payload) {
                                        bindType = *v.payload;
                                        auto p = next();
                                        emit(p + " = extractvalue { i64, i64 } " + value + ", 1");
                                        if (bindType.kind == TypeKind::String || bindType.kind == TypeKind::Pointer) {
                                            auto c = next();
                                            emit(c + " = inttoptr i64 " + p + " to ptr");
                                            bindValue = c;
                                        } else if (bindType.kind == TypeKind::Bool) {
                                            auto c = next();
                                            emit(c + " = trunc i64 " + p + " to i1");
                                            bindValue = c;
                                        } else if (bindType.kind == TypeKind::F64) {
                                            auto c = next();
                                            emit(c + " = bitcast i64 " + p + " to double");
                                            bindValue = c;
                                        } else {
                                            bindValue = p;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        auto ptr = next();
                        vars[arm.binding] = ptr;
                        types[arm.binding] = bindType;
                        emit(ptr + " = alloca " + llvmType(bindType));
                        emit("store " + llvmType(bindType) + " " + bindValue + ", ptr " + ptr);
                    }
                    const std::string matched =
                        arm.guard ? lab("match.guard") : bodies[k];
                    if (arm.wildcard || (!arm.binding.empty() && !arm.pattern)) {
                        emit("br label %" + matched);
                    } else {
                        auto patVal = expr(arm.pattern.get());
                        std::string pat = patVal;
                        if (vt.kind == TypeKind::Enum && dynamic_cast<const EnumVariantExpr*>(arm.pattern.get())) {
                            auto t = next();
                            emit(t + " = extractvalue { i64, i64 } " + patVal + ", 0");
                            pat = t;
                        }
                        if (arm.rangeEnd) {
                            auto upper = expr(arm.rangeEnd.get());
                            auto lowerOk = next();
                            auto upperOk = next();
                            emit(lowerOk + " = icmp sge i64 " + valueTag + ", " + pat);
                            emit(upperOk + " = icmp sle i64 " + valueTag + ", " + upper);
                            auto cmp = next();
                            emit(cmp + " = and i1 " + lowerOk + ", " + upperOk);
                            emit("br i1 " + cmp + ", label %" + matched + ", label %" +
                                 tests[k]);
                        } else {
                            auto cmp = next();
                            if (vt.kind == TypeKind::Bool)
                                emit(cmp + " = icmp eq i1 " + value + ", " + pat);
                            else
                                emit(cmp + " = icmp eq i64 " + valueTag + ", " + pat);
                            emit("br i1 " + cmp + ", label %" + matched + ", label %" +
                                 tests[k]);
                        }
                    }
                    if (arm.guard) {
                        emit(matched + ":");
                        auto guard = expr(arm.guard.get());
                        emit("br i1 " + guard + ", label %" + bodies[k] + ", label %" +
                             tests[k]);
                    }
                    emit(bodies[k] + ":");
                    terminated = false;
                    stmtBlock(arm.body.get(), ret);
                    if (!terminated) {
                        allTerminated = false;
                        emit("br label %" + end);
                    }
                    if ((arm.wildcard || (!arm.binding.empty() && !arm.pattern)) && !arm.guard)
                        hasCatchAll = true;
                    terminated = false;
                    vars = savedVars;
                    types = savedTypes;
                    if (k + 1 < mm->arms.size())
                        current = tests[k];
                }
                if (hasCatchAll && allTerminated) {
                    terminated = true;
                    return true;
                }
                emit(end + ":");
                terminated = false;
                return false;
            }
            if (dynamic_cast<const BreakStmt*>(s)) {
                if (breakLabels.empty() || loopDeferScopeDepth.empty())
                    throw std::runtime_error("break outside loop");
                emitDefersFrom(loopDeferScopeDepth.back());
                emit("br label %" + breakLabels.back());
                terminated = true;
                return true;
            }
            if (dynamic_cast<const ContinueStmt*>(s)) {
                if (continueLabels.empty() || loopDeferScopeDepth.empty())
                    throw std::runtime_error("continue outside loop");
                emitDefersFrom(loopDeferScopeDepth.back());
                emit("br label %" + continueLabels.back());
                terminated = true;
                return true;
            }
            return false;
        }
        std::string generate(const Program& p) {

            for (auto& s : const_cast<Program&>(p).structs)
                structs[s.name] = &s;
            for (auto& e : const_cast<Program&>(p).enums)
                enums[e.name] = &e;
            for (auto& f : const_cast<Program&>(p).functions)
                functions[f.name] = &f;

            emitModule("; Nexus LLVM IR v1.0.0");
            if (!options.targetTriple.empty())
                emitModule("target triple = \"" + options.targetTriple + "\"");

            for (auto& s : p.structs) {
                std::ostringstream t;
                t << "%struct." << s.name << " = type { ";
                for (size_t i = 0; i < s.fields.size(); ++i) {
                    if (i)
                        t << ", ";
                    t << llvmType(s.fields[i].type);
                }
                t << " }";
                emitModule(t.str());
            }
            emitModule("declare void @nexus_print_i64(i64)");
            emitModule("declare void @nexus_print_f64(double)");
            emitModule("declare void @nexus_print_bool(i1)");
            emitModule("declare void @nexus_print_str(ptr)");
            emitModule("declare ptr @nexus_str_concat(ptr, ptr)");
            emitModule("declare i1 @nexus_str_equal(ptr, ptr)");
            emitModule("declare void @nexus_bounds_check(i64, i64)");
            emitModule("declare ptr @nexus_input(ptr)");
            emitModule("declare i64 @nexus_input_i64(ptr)");
            emitModule("declare double @nexus_input_f64(ptr)");
            emitModule("declare ptr @nexus_str_i64(i64)");
            emitModule("declare ptr @nexus_str_f64(double)");
            emitModule("declare ptr @nexus_str_bool(i1)");
            emitModule("declare i64 @nexus_read_key()");
            emitModule("declare i1 @nexus_key_pressed()");
            emitModule("declare void @nexus_clear()");
            emitModule("declare void @nexus_sleep_ms(i64)");
            emitModule("declare i64 @nexus_random_i64(i64, i64)");
            emitModule("declare i64 @nexus_time_ms()");
            emitModule("declare i64 @nexus_system(ptr)");
            emitModule("declare ptr @nexus_capture_output(ptr)");
            emitModule("declare ptr @nexus_file_read(ptr)");
            emitModule("declare i1 @nexus_file_read_checked(ptr, ptr)");
            emitModule("declare i64 @nexus_file_write(ptr, ptr)");
            emitModule("declare i1 @nexus_file_exists(ptr)");
            emitModule("declare ptr @nexus_env(ptr)");
            emitModule("declare void @nexus_exit(i64)");
            emitModule("declare void @nexus_screen_begin()");
            emitModule("declare void @nexus_screen_end()");
            emitModule("declare void @nexus_screen_clear()");
            emitModule("declare void @nexus_screen_put(i64, i64, ptr)");
            emitModule("declare void @nexus_screen_present()");
            emitModule("declare void @nexus_screen_set_title(ptr)");
            emitModule("declare i64 @nexus_screen_width()");
            emitModule("declare i64 @nexus_screen_height()");
            emitModule("declare void @nexus_beep()");
            emitModule("declare i64 @nexus_mem_alloc(i64)");
            emitModule("declare void @nexus_mem_free(i64)");
            emitModule("declare void @nexus_mem_set(i64,i64,i64)");
            emitModule("declare i64 @nexus_mem_read(i64)");
            emitModule("declare void @nexus_mem_copy(i64,i64,i64)");
            emitModule("declare ptr @nexus_http_get(ptr)");
            emitModule("declare ptr @nexus_json_get(ptr, ptr)");
            emitModule("declare i64 @nexus_sqlite_open(ptr)");
            emitModule("declare i64 @nexus_sqlite_exec(i64, ptr)");
            emitModule("declare void @nexus_sqlite_close(i64)");
            emitModule("declare ptr @nexus_sha256(ptr)");
            emitModule("declare i64 @nexus_os_args_count()");
            emitModule("declare ptr @nexus_os_args_get(i64)");
            emitModule("declare i64 @nexus_string_len(ptr)");
            emitModule("declare i64 @nexus_string_byte_at(ptr, i64)");
            emitModule("declare i64 @nexus_string_char_count(ptr)");
            emitModule("declare i64 @nexus_string_char_at(ptr, i64)");
            emitModule("declare ptr @nexus_string_from_char(i64)");
            emitModule("declare i1 @nexus_string_valid_utf8(ptr)");
            emitModule("declare i1 @nexus_string_contains(ptr, ptr)");
            emitModule("declare i64 @nexus_string_find(ptr, ptr)");
            emitModule("declare ptr @nexus_string_trim(ptr)");
            emitModule("declare ptr @nexus_string_upper(ptr)");
            emitModule("declare ptr @nexus_string_lower(ptr)");
            emitModule("declare ptr @nexus_string_slice(ptr, i64, i64)");
            emitModule("declare i64 @nexus_string_split_count(ptr, ptr)");
            emitModule("declare ptr @nexus_string_split_at(ptr, ptr, i64)");
            emitModule("declare i64 @nexus_ilist_new()");
            emitModule("declare void @nexus_ilist_push(i64, i64)");
            emitModule("declare i64 @nexus_ilist_len(i64)");
            emitModule("declare i64 @nexus_ilist_get(i64, i64)");
            emitModule("declare void @nexus_ilist_set(i64, i64, i64)");
            emitModule("declare i64 @nexus_slist_new()");
            emitModule("declare void @nexus_slist_push(i64, ptr)");
            emitModule("declare i64 @nexus_slist_len(i64)");
            emitModule("declare ptr @nexus_slist_get(i64, i64)");
            emitModule("declare i1 @nexus_ilist_push_checked(i64, i64)");
            emitModule("declare i1 @nexus_ilist_reserve(i64, i64)");
            emitModule("declare i1 @nexus_ilist_clear(i64)");
            emitModule("declare i1 @nexus_ilist_free(i64)");
            emitModule("declare i1 @nexus_ilist_insert(i64, i64, i64)");
            emitModule("declare i1 @nexus_ilist_try_get(i64, i64, ptr)");
            emitModule("declare i1 @nexus_ilist_set_checked(i64, i64, i64)");
            emitModule("declare i1 @nexus_ilist_remove(i64, i64, ptr)");
            emitModule("declare i1 @nexus_ilist_try_pop(i64, ptr)");
            emitModule("declare i1 @nexus_slist_push_checked(i64, ptr)");
            emitModule("declare i1 @nexus_slist_reserve(i64, i64)");
            emitModule("declare i1 @nexus_slist_clear(i64)");
            emitModule("declare i1 @nexus_slist_free(i64)");
            emitModule("declare i1 @nexus_slist_insert(i64, i64, ptr)");
            emitModule("declare i1 @nexus_slist_try_get(i64, i64, ptr)");
            emitModule("declare i1 @nexus_slist_set_checked(i64, i64, ptr)");
            emitModule("declare i1 @nexus_slist_remove(i64, i64, ptr)");
            emitModule("declare i1 @nexus_slist_try_pop(i64, ptr)");

            emitModule("declare i1 @nexus_gfx_init(i64, i64, ptr)");

            emitModule("declare ptr @nexus_gfx_error()");

            emitModule("declare void @nexus_gfx_shutdown()");

            emitModule("declare i1 @nexus_gfx_should_close()");

            emitModule("declare void @nexus_gfx_poll()");

            emitModule("declare i64 @nexus_gfx_event_type()");

            emitModule("declare i64 @nexus_gfx_event_key()");

            emitModule("declare i64 @nexus_gfx_event_mouse_button()");

            emitModule("declare i64 @nexus_gfx_event_x()");

            emitModule("declare i64 @nexus_gfx_event_y()");

            emitModule("declare i1 @nexus_gfx_event_is(ptr)");

            emitModule("declare i64 @nexus_gfx_width()");

            emitModule("declare i64 @nexus_gfx_height()");

            emitModule("declare void @nexus_gfx_set_ui_resolution(double,double)");

            emitModule("declare i1 @nexus_gfx_vsync(i1)");

            emitModule("declare double @nexus_gfx_mouse_x()");

            emitModule("declare double @nexus_gfx_mouse_y()");

            emitModule("declare i1 @nexus_gfx_mouse_down(i64)");

            emitModule("declare i1 @nexus_gfx_mouse_clicked(i64)");

            emitModule("declare i1 @nexus_gfx_key_down(ptr)");

            emitModule("declare double @nexus_gfx_dt()");

            emitModule("declare void @nexus_gfx_begin()");

            emitModule("declare void @nexus_gfx_end()");

            emitModule("declare void @nexus_gfx_clear(double,double,double,double)");

            emitModule("declare void "
                       "@nexus_gfx_rect(double,double,double,double,double,double,double,double)");

            emitModule(
                "declare void @nexus_gfx_circle(double,double,double,double,double,double,double)");

            emitModule(
                "declare void "
                "@nexus_gfx_line(double,double,double,double,double,double,double,double,double)");

            emitModule("declare void "
                       "@nexus_gfx_text(double,double,double,ptr,double,double,double,double)");

            emitModule("declare void @nexus_gfx_set_title(ptr)");

            emitModule("declare void @nexus_gfx_fullscreen(i1)");
            emitModule("declare void @nexus_gfx_maximize()");
            emitModule("declare void @nexus_gfx_restore()");
            emitModule("declare i1 @nexus_gfx_is_fullscreen()");
            emitModule("declare ptr @nexus_gfx_backend()");

            emitModule("declare i64 @nexus_gfx_texture_load(ptr)");

            emitModule("declare void "
                       "@nexus_gfx_texture_draw(i64,double,double,double,double,double,double,"
                       "double,double)");

            emitModule("declare void @nexus_gfx_texture_unload(i64)");

            emitModule("declare i64 @nexus_gfx_texture_width(i64)");

            emitModule("declare i64 @nexus_gfx_texture_height(i64)");

            emitModule("declare void "
                       "@nexus_gfx3d_begin(double,double,double,double,double,double,double,double,"
                       "double)");

            emitModule("declare void "
                       "@nexus_gfx3d_cube(double,double,double,double,double,double,double,double,"
                       "double,double)");

            emitModule("declare void @nexus_gfx3d_grid(i64,double,double,double,double,double)");

            emitModule("declare void @nexus_gfx3d_end()");

            emitModule("declare i64 @nexus_gfx_sound_load(ptr)");

            emitModule("declare void @nexus_gfx_sound_play(i64,i1)");

            emitModule("declare void @nexus_gfx_sound_stop()");

            emitModule("declare void @nexus_gfx_sound_unload(i64)");

            emitModule("declare i64 @nexus_gfx_audio_available()");

            emitModule("declare ptr @nexus_gfx_audio_error()");

            emitModule("declare void @nexus_gfx_sound_volume(double)");

            emitModule("declare i1 @nexus_gfx_sound_playing()");

            emitModule("declare void @nexus_gfx_sound_tone(double,i64,double)");

            emitModule("declare double @nexus_sqrt(double)");

            emitModule("declare double @nexus_sin(double)");
            emitModule("declare double @nexus_cos(double)");
            emitModule("declare double @nexus_tan(double)");
            emitModule("declare double @nexus_atan2(double, double)");
            emitModule("declare double @nexus_floor(double)");
            emitModule("declare double @nexus_ceil(double)");
            emitModule("declare double @nexus_pow(double, double)");

            emitModule("declare double @nexus_to_f64(i64)");

            emitModule("declare i64 @nexus_to_i64(double)");

            emitModule("declare i64 @nexus_gfx_canvas_create(i64,i64,double,double,double,double)");

            emitModule("declare void @nexus_gfx_canvas_destroy(i64)");

            emitModule("declare void @nexus_gfx_canvas_clear(i64,double,double,double,double)");

            emitModule(
                "declare void "
                "@nexus_gfx_canvas_brush(i64,double,double,double,double,double,double,double)");

            emitModule("declare void "
                       "@nexus_gfx_canvas_line(i64,double,double,double,double,double,double,"
                       "double,double,double)");

            emitModule("declare void "
                       "@nexus_gfx_canvas_rect(i64,double,double,double,double,double,double,"
                       "double,double,double,i1)");

            emitModule(
                "declare void @nexus_gfx_canvas_fill(i64,i64,i64,double,double,double,double)");

            emitModule(
                "declare void @nexus_gfx_canvas_draw(i64,double,double,double,double,double)");

            emitModule("declare i1 @nexus_gfx_canvas_save_bmp(i64,ptr)");

            emitModule("declare i64 @nexus_gfx_canvas_load_bmp(ptr)");

            emitModule("declare void @nexus_gfx_canvas_checkpoint(i64)");

            emitModule("declare i1 @nexus_gfx_canvas_undo(i64)");

            emitModule("declare i1 @nexus_gfx_canvas_redo(i64)");

            emitModule("declare i64 @nexus_gfx_canvas_width(i64)");

            emitModule("declare i64 @nexus_gfx_canvas_height(i64)");

            emitModule("declare i64 @nexus_gfx3d_model_load(ptr)");

            emitModule("declare void "
                       "@nexus_gfx3d_model_draw(i64,double,double,double,double,double,double,"
                       "double,double,double)");

            emitModule("declare void @nexus_gfx3d_model_unload(i64)");

            emitModule("declare i64 "
                       "@nexus_gfx_particle_create(double,double,double,double,double,double,"
                       "double,double,double,double)");

            emitModule("declare i1 @nexus_gfx_particle_alive(i64)");

            emitModule("declare void @nexus_gfx_particles_update(double)");

            emitModule("declare void @nexus_gfx_particles_draw()");

            emitModule("declare void @nexus_gfx_particle_destroy(i64)");

            emitModule("declare void @nexus_gfx_particles_clear()");

            emitModule("declare void @nexus_gfx_camera_set(double,double,double)");

            emitModule("declare void @nexus_gfx_camera_reset()");

            emitModule("declare void "
                       "@nexus_gfx_texture_draw_frame(i64,double,double,double,double,i64,i64,i64,"
                       "i64,double,double,double,double)");

            emitModule("declare ptr @nexus_gfx_image_type()");

            emitModule("declare i64 @nexus_gfx_font_load(ptr,i64)");

            emitModule("declare void "
                       "@nexus_gfx_text_font(i64,double,double,ptr,double,double,double,double)");

            emitModule("declare void @nexus_gfx_font_unload(i64)");
            emitModule("declare i64 @nexus_gfx_font_default(i64)");
            emitModule("declare i64 @nexus_gfx_text_width(i64, ptr)");
            emitModule("declare i64 @nexus_gfx_text_height(i64)");
            emitModule("declare void @nexus_gfx_font_style(i64, i64)");

            emitModule("declare i64 @nexus_gfx_music_load(ptr)");

            emitModule("declare i1 @nexus_gfx_music_play(i64,i1)");

            emitModule("declare void @nexus_gfx_music_stop()");

            emitModule("declare void @nexus_gfx_music_volume(double)");

            emitModule("declare void @nexus_gfx_cursor_visible(i1)");

            emitModule("declare i1 @nexus_ui_button(double,double,double,double,ptr)");

            emitModule("declare void "
                       "@nexus_ui_panel(double,double,double,double,double,double,double,double)");

            emitModule("declare void @nexus_ui_label(double,double,ptr,double)");

            emitModule(
                "declare void "
                "@nexus_ui_progress(double,double,double,double,double,double,double,double)");

            emitModule("declare i64 @nexus_ecs_create()");

            emitModule("declare void @nexus_ecs_destroy(i64)");

            emitModule("declare i1 @nexus_ecs_alive(i64)");

            emitModule("declare void @nexus_ecs_set_position(i64,double,double)");

            emitModule("declare void @nexus_ecs_set_velocity(i64,double,double)");

            emitModule("declare void @nexus_ecs_set_size(i64,double,double)");

            emitModule("declare void @nexus_ecs_set_color(i64,double,double,double,double)");

            emitModule("declare void @nexus_ecs_set_texture(i64,i64)");

            emitModule("declare void @nexus_ecs_update(double)");

            emitModule("declare void @nexus_ecs_draw()");

            emitModule("declare i1 @nexus_ecs_collides(i64,i64)");

            emitModule("declare double @nexus_ecs_x(i64)");

            emitModule("declare double @nexus_ecs_y(i64)");

            emitModule("declare i64 @nexus_ecs_count()");

            emitModule("declare void @nexus_gfx_texture_filter(i64,i1)");

            emitModule("declare i64 @nexus_gfx_anim_create(i64,i64,i64,i64,i64,double,i1)");

            emitModule("declare void @nexus_gfx_anim_update(i64,double)");

            emitModule("declare void "
                       "@nexus_gfx_anim_draw(i64,double,double,double,double,double,double,double,"
                       "double,double)");

            emitModule("declare i64 @nexus_gfx_anim_frame(i64)");

            emitModule("declare void @nexus_gfx_anim_destroy(i64)");

            emitModule(
                "declare i1 "
                "@nexus_physics_aabb(double,double,double,double,double,double,double,double)");

            emitModule(
                "declare i1 @nexus_physics_circle(double,double,double,double,double,double)");

            emitModule("declare i64 @nexus_scene_create()");

            emitModule("declare void @nexus_scene_add(i64,i64)");

            emitModule("declare i1 @nexus_scene_save(i64,ptr)");

            emitModule("declare i64 @nexus_scene_load(ptr)");

            emitModule("declare void @nexus_scene_destroy(i64)");

            emitModule("declare void @nexus_scene_clear(i64)");

            emitModule("declare i1 @nexus_ui_checkbox(double,double,ptr,i1)");

            emitModule("declare double @nexus_ui_slider(double,double,double,double,double)");

            for (auto& f : const_cast<Program&>(p).functions) {
                if (f.external) {
                    std::ostringstream s;
                    s << "declare " << llvmType(f.ret) << " @" << f.name << "(";
                    for (size_t i = 0; i < f.params.size(); ++i) {
                        if (i)
                            s << ", ";
                        s << llvmType(f.params[i].type);
                    }
                    s << ")";
                    emitModule(s.str());
                    continue;
                }
                vars.clear();
                types.clear();
                reg = 0;
                label = 0;
                terminated = false;
                breakLabels.clear();
                continueLabels.clear();
                deferredCalls.clear();
                loopDeferScopeDepth.clear();
                body.str("");
                body.clear();
                Type codegenRet = f.ret;
                if (f.name == "main" && codegenRet.kind == TypeKind::Void)
                    codegenRet = Type::i64();
                currentReturnType = codegenRet;
                std::ostringstream sig;
                sig << "define " << llvmType(codegenRet) << " @" << f.name << "(";
                for (size_t i = 0; i < f.params.size(); ++i) {
                    if (i)
                        sig << ", ";
                    sig << llvmType(f.params[i].type) << " %arg" << i;
                }
                sig << ") {";
                emit(sig.str());
                emit("entry:");
                for (size_t i = 0; i < f.params.size(); ++i) {
                    auto ptr = next();
                    vars[f.params[i].name] = ptr;
                    types[f.params[i].name] = f.params[i].type;
                    emit(ptr + " = alloca " + llvmType(f.params[i].type));
                    emit("store " + llvmType(f.params[i].type) + " %arg" + std::to_string(i) +
                         ", ptr " + ptr);
                }
                stmtBlock(f.body.get(), codegenRet);
                if (!terminated) {
                    if (codegenRet.kind == TypeKind::Void)
                        emit("ret void");
                    else if (f.name == "main")
                        emit("ret " + llvmType(codegenRet) + " 0");
                    else
                        throw std::runtime_error("function may fall through without return: " +
                                                 f.name);
                }
                emit("}");
                functionsIR << body.str();
            }
            std::ostringstream result;
            result << module.str();
            for (auto& g : globals)
                result << g << '\n';
            result << functionsIR.str();
            return result.str();
        }
    };

    std::string LLVMIRGenerator::generate(const Program& p) {
        return CG(options_).generate(p);
    }
} // namespace nexus
