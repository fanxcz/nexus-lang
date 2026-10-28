#include "codegen/llvm_cg.hpp"

#include <stdexcept>

namespace nexus {

    Type CG::typeOf(const Expr* e) {

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

        if (auto* tr = dynamic_cast<const TryExpr*>(e)) {
            const auto inner = typeOf(tr->expr.get());
            if (inner.kind != TypeKind::Enum)
                throw std::runtime_error("try expression on non-enum in codegen");
            const auto& en = getEnum(inner.name);
            for (const auto& variant : en.variants)
                if ((variant.name == "Some" || variant.name == "Ok") && variant.payload)
                    return *variant.payload;
            throw std::runtime_error("try enum has no success payload in codegen");
        }

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
    std::string CG::stringValue(const std::string& s) {
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
    std::string CG::loadVar(const std::string& name) {
        auto r = next();
        auto t = types.at(name);
        emit(r + " = load " + llvmType(t) + ", ptr " + vars.at(name));
        return r;
    }
    std::string CG::addressOf(const Expr* e) {

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
    std::string CG::memberLoad(const MemberExpr* m) {
        auto addr = addressOf(m);
        auto t = typeOf(m);
        auto r = next();
        emit(r + " = load " + llvmType(t) + ", ptr " + addr);
        return r;
    }
    std::string CG::expr(const Expr* e) {

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
            const auto innerType = typeOf(t->expr.get());
            if (innerType.kind != TypeKind::Enum)
                throw std::runtime_error("try expression on non-enum in codegen");
            const auto& en = getEnum(innerType.name);
            const EnumVariant* success = nullptr;
            long long successTag = -1;
            for (std::size_t i = 0; i < en.variants.size(); ++i) {
                const auto& variant = en.variants[i];
                if ((variant.name == "Some" || variant.name == "Ok") && variant.payload) {
                    success = &variant;
                    successTag = static_cast<long long>(i);
                    break;
                }
            }
            if (!success || successTag < 0)
                throw std::runtime_error("try enum has no success variant in codegen");

            const auto inner = expr(t->expr.get());
            const auto tag = next();
            emit(tag + " = extractvalue { i64, i64 } " + inner + ", 0");
            const auto cmp = next();
            emit(cmp + " = icmp eq i64 " + tag + ", " + std::to_string(successTag));
            const auto errLabel = lab("try.err");
            const auto okLabel = lab("try.ok");
            emit("br i1 " + cmp + ", label %" + okLabel + ", label %" + errLabel);
            startBlock(errLabel);
            // Error propagation is an early return just like an explicit
            // return statement, so every active lexical defer must run.
            emitDefersFrom(0);
            emit("ret " + llvmType(currentReturnType) + " " + inner);
            terminated = true;
            startBlock(okLabel);
            terminated = false;

            const auto payload = next();
            emit(payload + " = extractvalue { i64, i64 } " + inner + ", 1");
            const auto payloadType = *success->payload;
            if (payloadType.kind == TypeKind::I64)
                return payload;
            if (payloadType.kind == TypeKind::Bool) {
                const auto value = next();
                emit(value + " = trunc i64 " + payload + " to i1");
                return value;
            }
            if (payloadType.kind == TypeKind::F64) {
                const auto value = next();
                emit(value + " = bitcast i64 " + payload + " to double");
                return value;
            }
            if (payloadType.kind == TypeKind::String || payloadType.kind == TypeKind::Pointer) {
                const auto value = next();
                emit(value + " = inttoptr i64 " + payload + " to ptr");
                return value;
            }
            throw std::runtime_error("unsupported try payload type in codegen");
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
                else if (payloadType.kind == TypeKind::Bool) {
                    auto c = next();
                    emit(c + " = zext i1 " + val + " to i64");
                    payloadI64 = c;
                } else if (payloadType.kind == TypeKind::String || payloadType.kind == TypeKind::Pointer) {
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

            // `&&` and `||` short-circuit: the right-hand side is only
            // evaluated when the left-hand side does not already decide the
            // result. Evaluating both sides eagerly changes observable
            // behaviour (side effects, guards around division by zero, ...).
            if (b->op == TokenKind::AndAnd || b->op == TokenKind::OrOr) {
                const bool isAnd = b->op == TokenKind::AndAnd;
                auto left = expr(b->lhs.get());
                const auto lhsBlock = currentBlock;
                const auto rhsBlock = lab("logic.rhs");
                const auto endBlock = lab("logic.end");
                if (isAnd)
                    emit("br i1 " + left + ", label %" + rhsBlock + ", label %" + endBlock);
                else
                    emit("br i1 " + left + ", label %" + endBlock + ", label %" + rhsBlock);
                startBlock(rhsBlock);
                auto right = expr(b->rhs.get());
                const auto rhsEndBlock = currentBlock;
                emit("br label %" + endBlock);
                startBlock(endBlock);
                auto result = next();
                emit(result + " = phi i1 [ " + std::string(isAnd ? "false" : "true") +
                     ", %" + lhsBlock + " ], [ " + right + ", %" + rhsEndBlock + " ]");
                return result;
            }

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
            // Allocate any auxiliary integer result before the final result.
            // LLVM textual IR requires numbered SSA values to appear in order.
            std::string auxiliary;
            if (t.kind == TypeKind::I64 &&
                (b->op == TokenKind::Slash || b->op == TokenKind::Percent ||
                 b->op == TokenKind::ShiftLeft || b->op == TokenKind::ShiftRight))
                auxiliary = next();
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
            case TokenKind::Slash: {
                emit(auxiliary + " = call i64 @nexus_division_check(i64 " + l + ", i64 " + rv + ")");
                emit(r + " = sdiv i64 " + l + ", " + auxiliary);
                break;
            }
            case TokenKind::Percent: {
                emit(auxiliary + " = call i64 @nexus_division_check(i64 " + l + ", i64 " + rv + ")");
                emit(r + " = srem i64 " + l + ", " + auxiliary);
                break;
            }
            case TokenKind::Ampersand:
                emit(r + " = and i64 " + l + ", " + rv);
                break;
            case TokenKind::Pipe:
                emit(r + " = or i64 " + l + ", " + rv);
                break;
            case TokenKind::Caret:
                emit(r + " = xor i64 " + l + ", " + rv);
                break;
            case TokenKind::ShiftLeft: {
                // Match the builtin x86-64 backend: only the low six bits of an
                // i64 shift count are significant. LLVM otherwise treats counts
                // >= 64 as poison, causing backend-dependent program behavior.
                emit(auxiliary + " = and i64 " + rv + ", 63");
                emit(r + " = shl i64 " + l + ", " + auxiliary);
                break;
            }
            case TokenKind::ShiftRight: {
                emit(auxiliary + " = and i64 " + rv + ", 63");
                emit(r + " = ashr i64 " + l + ", " + auxiliary);
                break;
            }
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
    std::string CG::deferredCall(const CallExpr* c) {
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
    void CG::emitDefersFrom(std::size_t keepDepth) {
        for (std::size_t depth = deferredCalls.size(); depth > keepDepth; --depth) {
            const auto& calls = deferredCalls[depth - 1];
            for (auto it = calls.rbegin(); it != calls.rend(); ++it)
                emit(*it);
        }
    }

        // Emit `lhs <op> rhs` when both operands have already been evaluated.
        // Used by compound assignment, where the left operand is a loaded value
        // rather than an expression tree.
    std::string CG::compoundValue(TokenKind op, const Type& type, const std::string& lhs,
                              const std::string& rhs) {
        if (type.kind == TypeKind::String) {
            if (op != TokenKind::Plus)
                throw std::runtime_error("unsupported compound assignment for string");
            auto r = next();
            emit(r + " = call ptr @nexus_str_concat(ptr " + lhs + ", ptr " + rhs + ")");
            return r;
        }
        if (type.kind == TypeKind::F64) {
            auto r = next();
            switch (op) {
            case TokenKind::Plus:
                emit(r + " = fadd double " + lhs + ", " + rhs);
                return r;
            case TokenKind::Minus:
                emit(r + " = fsub double " + lhs + ", " + rhs);
                return r;
            case TokenKind::Star:
                emit(r + " = fmul double " + lhs + ", " + rhs);
                return r;
            case TokenKind::Slash:
                emit(r + " = fdiv double " + lhs + ", " + rhs);
                return r;
            default:
                throw std::runtime_error("unsupported compound assignment for f64");
            }
        }
        // As in binary expressions, reserve the auxiliary SSA value before
        // the instruction that consumes it, preserving valid LLVM numeric IDs.
        const bool needsAuxiliary = op == TokenKind::Slash || op == TokenKind::Percent ||
                                    op == TokenKind::ShiftLeft || op == TokenKind::ShiftRight;
        const auto auxiliary = needsAuxiliary ? next() : std::string{};
        auto r = next();
        switch (op) {
        case TokenKind::Plus:
            emit(r + " = add i64 " + lhs + ", " + rhs);
            return r;
        case TokenKind::Minus:
            emit(r + " = sub i64 " + lhs + ", " + rhs);
            return r;
        case TokenKind::Star:
            emit(r + " = mul i64 " + lhs + ", " + rhs);
            return r;
        case TokenKind::Slash: {
            emit(auxiliary + " = call i64 @nexus_division_check(i64 " + lhs + ", i64 " + rhs + ")");
            emit(r + " = sdiv i64 " + lhs + ", " + auxiliary);
            return r;
        }
        case TokenKind::Percent: {
            emit(auxiliary + " = call i64 @nexus_division_check(i64 " + lhs + ", i64 " + rhs + ")");
            emit(r + " = srem i64 " + lhs + ", " + auxiliary);
            return r;
        }
        case TokenKind::Ampersand:
            emit(r + " = and i64 " + lhs + ", " + rhs);
            return r;
        case TokenKind::Pipe:
            emit(r + " = or i64 " + lhs + ", " + rhs);
            return r;
        case TokenKind::Caret:
            emit(r + " = xor i64 " + lhs + ", " + rhs);
            return r;
        case TokenKind::ShiftLeft: {
            emit(auxiliary + " = and i64 " + rhs + ", 63");
            emit(r + " = shl i64 " + lhs + ", " + auxiliary);
            return r;
        }
        case TokenKind::ShiftRight: {
            emit(auxiliary + " = and i64 " + rhs + ", 63");
            emit(r + " = ashr i64 " + lhs + ", " + auxiliary);
            return r;
        }
        default:
            throw std::runtime_error("unsupported compound assignment operator");
        }
    }

} // namespace nexus
