#include "codegen/llvm_cg.hpp"

#include <stdexcept>

namespace nexus {

    Type CG::typeOf(const Expr* e) {

        if (const auto* literal = dynamic_cast<const IntExpr*>(e))
            return literal->literalType;
        if (const auto* literal = dynamic_cast<const FloatExpr*>(e))
            return literal->literalType;
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
            if ((t.kind != TypeKind::Array && t.kind != TypeKind::Slice &&
                 t.kind != TypeKind::MutableSlice) || !t.element)
                throw std::runtime_error("index on non-array or slice");
            return *t.element;
        }
        if (auto* slice = dynamic_cast<const SliceExpr*>(e)) {
            auto t = typeOf(slice->object.get());
            if ((t.kind != TypeKind::Array && t.kind != TypeKind::Slice &&
                 t.kind != TypeKind::MutableSlice) || !t.element)
                throw std::runtime_error("slice on non-array or slice");
            return Type::slice(*t.element);
        }
        if (auto* u = dynamic_cast<const UnaryExpr*>(e)) {
            auto t = typeOf(u->rhs.get());
            if (u->op == TokenKind::KwMut) {
                if (t.kind != TypeKind::Slice || !t.element)
                    throw std::runtime_error("mutable slice qualifier requires a slice expression");
                return Type::mutableSlice(*t.element);
            }
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
            if (c->callee == "len")
                return Type::i64();
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
            if (!ot.element || (ot.kind != TypeKind::Array && ot.kind != TypeKind::Slice &&
                 ot.kind != TypeKind::MutableSlice))
                throw std::runtime_error("index address requires array or slice");
            auto idx = expr(ix->index.get());
            std::string base;
            std::string length;
            if (ot.kind == TypeKind::Array) {
                base = addressOf(ix->object.get());
                length = std::to_string(ot.arraySize);
                emit("call void @nexus_bounds_check(i64 " + idx + ", i64 " + length + ")");
                auto r = next();
                emit(r + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                     ", i64 0, i64 " + idx);
                return r;
            }

            const auto sliceValue = expr(ix->object.get());
            base = next();
            emit(base + " = extractvalue { ptr, i64 } " + sliceValue + ", 0");
            length = next();
            emit(length + " = extractvalue { ptr, i64 } " + sliceValue + ", 1");
            emit("call void @nexus_bounds_check(i64 " + idx + ", i64 " + length + ")");
            auto r = next();
            emit(r + " = getelementptr inbounds " + llvmType(*ot.element) + ", ptr " + base +
                 ", i64 " + idx);
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

        if (auto* x = dynamic_cast<const IntExpr*>(e)) {
            const auto magnitude = std::to_string(x->value);
            return x->negative ? "-" + magnitude : magnitude;
        }

        if (auto* x = dynamic_cast<const FloatExpr*>(e)) {
            std::ostringstream o;
            o.setf(std::ios::scientific);
            o << std::setprecision(x->literalType.kind == TypeKind::F32 ? 9 : 17)
              << (x->literalType.kind == TypeKind::F32
                      ? static_cast<double>(static_cast<float>(x->value))
                      : x->value);
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
            if (payloadType.kind == TypeKind::I64 || payloadType.kind == TypeKind::U64)
                return payload;
            if (isIntegerType(payloadType.kind)) {
                const auto value = next();
                emit(value + " = trunc i64 " + payload + " to " + llvmType(payloadType));
                return value;
            }
            if (payloadType.kind == TypeKind::Bool) {
                const auto value = next();
                emit(value + " = trunc i64 " + payload + " to i1");
                return value;
            }
            if (payloadType.kind == TypeKind::F32) {
                const auto bits = next();
                emit(bits + " = trunc i64 " + payload + " to i32");
                const auto value = next();
                emit(value + " = bitcast i32 " + bits + " to float");
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
                if (payloadType.kind == TypeKind::I64 || payloadType.kind == TypeKind::U64)
                    payloadI64 = val;
                else if (isIntegerType(payloadType.kind)) {
                    auto c = next();
                    const auto extension = isSignedIntegerType(payloadType.kind) ? "sext" : "zext";
                    emit(c + " = " + extension + " " + llvmType(payloadType) + " " + val + " to i64");
                    payloadI64 = c;
                } else if (payloadType.kind == TypeKind::Bool) {
                    auto c = next();
                    emit(c + " = zext i1 " + val + " to i64");
                    payloadI64 = c;
                } else if (payloadType.kind == TypeKind::String || payloadType.kind == TypeKind::Pointer) {
                    auto c = next();
                    emit(c + " = ptrtoint ptr " + val + " to i64");
                    payloadI64 = c;
                } else if (payloadType.kind == TypeKind::F32) {
                    auto bits = next();
                    emit(bits + " = bitcast float " + val + " to i32");
                    auto c = next();
                    emit(c + " = zext i32 " + bits + " to i64");
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
        if (auto* slice = dynamic_cast<const SliceExpr*>(e)) {
            const auto objectType = typeOf(slice->object.get());
            if (!objectType.element ||
                (objectType.kind != TypeKind::Array && objectType.kind != TypeKind::Slice &&
                 objectType.kind != TypeKind::MutableSlice))
                throw std::runtime_error("slice lowering requires an array or slice source");

            std::string dataPointer;
            std::string sourceLength;
            if (objectType.kind == TypeKind::Array) {
                const auto arrayAddress = addressOf(slice->object.get());
                dataPointer = next();
                emit(dataPointer + " = getelementptr inbounds " + llvmType(objectType) +
                     ", ptr " + arrayAddress + ", i64 0, i64 0");
                sourceLength = std::to_string(objectType.arraySize);
            } else {
                const auto sourceValue = expr(slice->object.get());
                dataPointer = next();
                emit(dataPointer + " = extractvalue { ptr, i64 } " + sourceValue + ", 0");
                sourceLength = next();
                emit(sourceLength + " = extractvalue { ptr, i64 } " + sourceValue + ", 1");
            }

            const auto start = expr(slice->start.get());
            const auto end = expr(slice->end.get());
            emit("call void @nexus_slice_bounds_check(i64 " + start + ", i64 " + end +
                 ", i64 " + sourceLength + ")");

            const auto offsetPointer = next();
            emit(offsetPointer + " = getelementptr inbounds " + llvmType(*objectType.element) +
                 ", ptr " + dataPointer + ", i64 " + start);
            const auto sliceLength = next();
            emit(sliceLength + " = sub i64 " + end + ", " + start);

            std::string value = "undef";
            auto withPointer = next();
            emit(withPointer + " = insertvalue { ptr, i64 } " + value + ", ptr " +
                 offsetPointer + ", 0");
            auto withLength = next();
            emit(withLength + " = insertvalue { ptr, i64 } " + withPointer + ", i64 " +
                 sliceLength + ", 1");
            return withLength;
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
            if (u->op == TokenKind::KwMut)
                return a;
            if (u->op == TokenKind::Ampersand)
                return addressOf(u->rhs.get());
            auto r = next();
            if (u->op == TokenKind::Minus) {
                const auto irType = llvmType(t);
                if (isSignedIntegerType(t.kind))
                    emit(r + " = sub " + irType + " 0, " + a);
                else if (isFloatType(t.kind))
                    emit(r + " = fneg " + irType + " " + a);
                else
                    throw std::runtime_error("unary minus requires signed numeric type");
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
            const bool integer = isIntegerType(t.kind);
            const bool floating = isFloatType(t.kind);
            const std::string irType = llvmType(t);
            const bool needsDivisionCheck = integer &&
                (b->op == TokenKind::Slash || b->op == TokenKind::Percent);
            const bool needsShiftMask = integer &&
                (b->op == TokenKind::ShiftLeft || b->op == TokenKind::ShiftRight);
            std::string wideL, wideR, checkedR, divisorValue, shiftAmount;
            if (needsDivisionCheck) {
                wideL = next();
                wideR = next();
                checkedR = next();
                divisorValue = next();
            } else if (needsShiftMask) {
                shiftAmount = next();
            }
            auto r = next();

            if (floating) {
                switch (b->op) {
                case TokenKind::Plus: emit(r + " = fadd " + irType + " " + l + ", " + rv); break;
                case TokenKind::Minus: emit(r + " = fsub " + irType + " " + l + ", " + rv); break;
                case TokenKind::Star: emit(r + " = fmul " + irType + " " + l + ", " + rv); break;
                case TokenKind::Slash: emit(r + " = fdiv " + irType + " " + l + ", " + rv); break;
                case TokenKind::EqualEqual: emit(r + " = fcmp oeq " + irType + " " + l + ", " + rv); break;
                case TokenKind::BangEqual: emit(r + " = fcmp one " + irType + " " + l + ", " + rv); break;
                case TokenKind::Less: emit(r + " = fcmp olt " + irType + " " + l + ", " + rv); break;
                case TokenKind::LessEqual: emit(r + " = fcmp ole " + irType + " " + l + ", " + rv); break;
                case TokenKind::Greater: emit(r + " = fcmp ogt " + irType + " " + l + ", " + rv); break;
                case TokenKind::GreaterEqual: emit(r + " = fcmp oge " + irType + " " + l + ", " + rv); break;
                default: throw std::runtime_error("unsupported float operator");
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
            if (!integer)
                throw std::runtime_error("unsupported numeric type in codegen");

            const bool isSigned = isSignedIntegerType(t.kind);
            const unsigned bits = integerBitWidth(t.kind);
            const std::string compare = isSigned ? "s" : "u";
            if (needsDivisionCheck) {
                const std::string extend = isSigned ? "sext" : "zext";
                if (bits == 64) {
                    emit(wideL + " = add i64 " + l + ", 0");
                    emit(wideR + " = add i64 " + rv + ", 0");
                } else {
                    emit(wideL + " = " + extend + " " + irType + " " + l + " to i64");
                    emit(wideR + " = " + extend + " " + irType + " " + rv + " to i64");
                }
                emit(checkedR + " = call i64 @nexus_division_check_typed(i64 " + wideL +
                     ", i64 " + wideR + ", i64 " + std::to_string(bits) +
                     ", i1 " + (isSigned ? "true" : "false") + ")");
                if (bits == 64)
                    emit(divisorValue + " = add i64 " + checkedR + ", 0");
                else
                    emit(divisorValue + " = trunc i64 " + checkedR + " to " + irType);
            } else if (needsShiftMask) {
                emit(shiftAmount + " = and " + irType + " " + rv + ", " +
                     std::to_string(bits - 1));
            }

            switch (b->op) {
            case TokenKind::Plus: emit(r + " = add " + irType + " " + l + ", " + rv); break;
            case TokenKind::Minus: emit(r + " = sub " + irType + " " + l + ", " + rv); break;
            case TokenKind::Star: emit(r + " = mul " + irType + " " + l + ", " + rv); break;
            case TokenKind::Slash: emit(r + " = " + (isSigned ? "sdiv " : "udiv ") + irType + " " + l + ", " + divisorValue); break;
            case TokenKind::Percent: emit(r + " = " + (isSigned ? "srem " : "urem ") + irType + " " + l + ", " + divisorValue); break;
            case TokenKind::Ampersand: emit(r + " = and " + irType + " " + l + ", " + rv); break;
            case TokenKind::Pipe: emit(r + " = or " + irType + " " + l + ", " + rv); break;
            case TokenKind::Caret: emit(r + " = xor " + irType + " " + l + ", " + rv); break;
            case TokenKind::ShiftLeft: emit(r + " = shl " + irType + " " + l + ", " + shiftAmount); break;
            case TokenKind::ShiftRight: emit(r + " = " + (isSigned ? "ashr " : "lshr ") + irType + " " + l + ", " + shiftAmount); break;
            case TokenKind::EqualEqual: emit(r + " = icmp eq " + irType + " " + l + ", " + rv); break;
            case TokenKind::BangEqual: emit(r + " = icmp ne " + irType + " " + l + ", " + rv); break;
            case TokenKind::Less: emit(r + " = icmp " + compare + "lt " + irType + " " + l + ", " + rv); break;
            case TokenKind::LessEqual: emit(r + " = icmp " + compare + "le " + irType + " " + l + ", " + rv); break;
            case TokenKind::Greater: emit(r + " = icmp " + compare + "gt " + irType + " " + l + ", " + rv); break;
            case TokenKind::GreaterEqual: emit(r + " = icmp " + compare + "ge " + irType + " " + l + ", " + rv); break;
            default: throw std::runtime_error("unsupported integer operator");
            }
            return r;
        }
        if (auto* c = dynamic_cast<const CallExpr*>(e)) {
            auto it = functions.find(c->callee);
            if (c->callee == "len") {
                if (c->args.size() != 1)
                    throw std::runtime_error("len expects exactly one argument");
                const auto argumentType = typeOf(c->args[0].get());
                if (argumentType.kind == TypeKind::String) {
                    const auto value = expr(c->args[0].get());
                    const auto length = next();
                    emit(length + " = call i64 @nexus_string_len(ptr " + value + ")");
                    return length;
                }
                if (argumentType.kind == TypeKind::Array) {
                    (void)expr(c->args[0].get());
                    return std::to_string(argumentType.arraySize);
                }
                if (argumentType.kind != TypeKind::Slice &&
                    argumentType.kind != TypeKind::MutableSlice)
                    throw std::runtime_error("len expects an array or slice");
                const auto value = expr(c->args[0].get());
                const auto length = next();
                emit(length + " = extractvalue { ptr, i64 } " + value + ", 1");
                return length;
            }
            if (c->callee == "print") {
                auto av = expr(c->args[0].get());
                auto t = typeOf(c->args[0].get());
                if (isIntegerType(t.kind)) {
                    const auto bits = integerBitWidth(t.kind);
                    std::string value = av;
                    if (bits < 64) {
                        value = next();
                        emit(value + " = " + (isSignedIntegerType(t.kind) ? "sext " : "zext ") +
                             llvmType(t) + " " + av + " to i64");
                    }
                    if (t.kind == TypeKind::U64)
                        emit("call void @nexus_print_u64(i64 " + value + ")");
                    else
                        emit("call void @nexus_print_i64(i64 " + value + ")");
                } else if (isFloatType(t.kind)) {
                    std::string value = av;
                    if (t.kind == TypeKind::F32) {
                        value = next();
                        emit(value + " = fpext float " + av + " to double");
                    }
                    emit("call void @nexus_print_f64(double " + value + ")");
                }
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
            if (isIntegerType(type.kind)) {
                const auto bits = integerBitWidth(type.kind);
                if (bits == 64) {
                    if (type.kind == TypeKind::U64)
                        return "call void @nexus_print_u64(i64 " + value + ")";
                    return "call void @nexus_print_i64(i64 " + value + ")";
                }
                const auto wide = next();
                emit(wide + " = " + (isSignedIntegerType(type.kind) ? "sext " : "zext ") +
                     llvmType(type) + " " + value + " to i64");
                return "call void @nexus_print_i64(i64 " + wide + ")";
            }
            if (isFloatType(type.kind)) {
                if (type.kind == TypeKind::F64)
                    return "call void @nexus_print_f64(double " + value + ")";
                const auto wide = next();
                emit(wide + " = fpext float " + value + " to double");
                return "call void @nexus_print_f64(double " + wide + ")";
            }
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
        const std::string irType = llvmType(type);
        if (isFloatType(type.kind)) {
            auto r = next();
            switch (op) {
            case TokenKind::Plus: emit(r + " = fadd " + irType + " " + lhs + ", " + rhs); return r;
            case TokenKind::Minus: emit(r + " = fsub " + irType + " " + lhs + ", " + rhs); return r;
            case TokenKind::Star: emit(r + " = fmul " + irType + " " + lhs + ", " + rhs); return r;
            case TokenKind::Slash: emit(r + " = fdiv " + irType + " " + lhs + ", " + rhs); return r;
            default: throw std::runtime_error("unsupported compound assignment for float");
            }
        }
        if (!isIntegerType(type.kind))
            throw std::runtime_error("unsupported compound assignment type");

        const unsigned bits = integerBitWidth(type.kind);
        const bool isSigned = isSignedIntegerType(type.kind);
        const bool needsDivisionCheck = op == TokenKind::Slash || op == TokenKind::Percent;
        const bool needsShiftMask = op == TokenKind::ShiftLeft || op == TokenKind::ShiftRight;
        std::string wideL, wideR, checkedR, divisorValue, shiftAmount;
        if (needsDivisionCheck) {
            wideL = next(); wideR = next(); checkedR = next(); divisorValue = next();
            const std::string extend = isSigned ? "sext" : "zext";
            if (bits == 64) {
                emit(wideL + " = add i64 " + lhs + ", 0");
                emit(wideR + " = add i64 " + rhs + ", 0");
            } else {
                emit(wideL + " = " + extend + " " + irType + " " + lhs + " to i64");
                emit(wideR + " = " + extend + " " + irType + " " + rhs + " to i64");
            }
            emit(checkedR + " = call i64 @nexus_division_check_typed(i64 " + wideL +
                 ", i64 " + wideR + ", i64 " + std::to_string(bits) +
                 ", i1 " + (isSigned ? "true" : "false") + ")");
            if (bits == 64) emit(divisorValue + " = add i64 " + checkedR + ", 0");
            else emit(divisorValue + " = trunc i64 " + checkedR + " to " + irType);
        } else if (needsShiftMask) {
            shiftAmount = next();
            emit(shiftAmount + " = and " + irType + " " + rhs + ", " + std::to_string(bits - 1));
        }
        auto r = next();
        switch (op) {
        case TokenKind::Plus: emit(r + " = add " + irType + " " + lhs + ", " + rhs); return r;
        case TokenKind::Minus: emit(r + " = sub " + irType + " " + lhs + ", " + rhs); return r;
        case TokenKind::Star: emit(r + " = mul " + irType + " " + lhs + ", " + rhs); return r;
        case TokenKind::Slash: emit(r + " = " + (isSigned ? "sdiv " : "udiv ") + irType + " " + lhs + ", " + divisorValue); return r;
        case TokenKind::Percent: emit(r + " = " + (isSigned ? "srem " : "urem ") + irType + " " + lhs + ", " + divisorValue); return r;
        case TokenKind::Ampersand: emit(r + " = and " + irType + " " + lhs + ", " + rhs); return r;
        case TokenKind::Pipe: emit(r + " = or " + irType + " " + lhs + ", " + rhs); return r;
        case TokenKind::Caret: emit(r + " = xor " + irType + " " + lhs + ", " + rhs); return r;
        case TokenKind::ShiftLeft: emit(r + " = shl " + irType + " " + lhs + ", " + shiftAmount); return r;
        case TokenKind::ShiftRight: emit(r + " = " + (isSigned ? "ashr " : "lshr ") + irType + " " + lhs + ", " + shiftAmount); return r;
        default: throw std::runtime_error("unsupported compound assignment operator");
        }
    }


} // namespace nexus
