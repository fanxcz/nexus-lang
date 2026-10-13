#include "native/native.hpp"

#include "lexer/token.hpp"
#include "native/image.hpp"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace nexus::native {

    namespace detail {

        enum class Kind { I64, Bool, Void, Str };

        struct FnSig {
            std::vector<Kind> params;
            Kind ret = Kind::Void;
        };

        Kind checkExpr(const Expr* e,
                       const std::map<std::string, Kind>& env,
                       const std::map<std::string, FnSig>& fns,
                       std::string& reason);

        Kind checkCall(const CallExpr* c,
                       const std::map<std::string, Kind>& env,
                       const std::map<std::string, FnSig>& fns,
                       std::string& reason) {
            if (c->callee == "print") {
                if (c->args.size() != 1) {
                    reason = "print() with " + std::to_string(c->args.size()) +
                             " arguments is outside the builtin native v0 subset (exactly 1 "
                             "argument is supported)";
                    return Kind::Void;
                }
                const Kind a = checkExpr(c->args[0].get(), env, fns, reason);
                if (!reason.empty())
                    return Kind::Void;
                if (a != Kind::I64 && a != Kind::Bool && a != Kind::Str) {
                    reason = "print() of this type is outside the builtin native v0 subset "
                             "(only i64, bool, and string literals are supported)";
                    return Kind::Void;
                }
                return Kind::Void;
            }
            const auto it = fns.find(c->callee);
            if (it == fns.end()) {
                // Narrow single-purpose builtins with inline runtime helpers.
                if (c->callee == "str_i64") {
                    if (c->args.size() != 1) {
                        reason = "str_i64() takes exactly 1 argument";
                        return Kind::Void;
                    }
                    const Kind a = checkExpr(c->args[0].get(), env, fns, reason);
                    if (!reason.empty())
                        return Kind::Void;
                    if (a != Kind::I64) {
                        reason = "str_i64() takes an i64 argument";
                        return Kind::Void;
                    }
                    return Kind::Str;
                }
                if (c->callee == "input") {
                    if (c->args.size() != 1) {
                        reason = "input() takes exactly 1 argument (the prompt)";
                        return Kind::Void;
                    }
                    const Kind a = checkExpr(c->args[0].get(), env, fns, reason);
                    if (!reason.empty())
                        return Kind::Void;
                    if (a != Kind::Str) {
                        reason = "input() takes a string prompt";
                        return Kind::Void;
                    }
                    return Kind::Str;
                }
                if (c->callee == "os.args_count") {
                    if (c->args.size() != 0) {
                        reason = "os.args_count() takes no arguments";
                        return Kind::Void;
                    }
                    return Kind::I64;
                }
                if (c->callee == "os.args_get") {
                    if (c->args.size() != 1) {
                        reason = "os.args_get() takes exactly 1 argument (index)";
                        return Kind::Void;
                    }
                    const Kind a = checkExpr(c->args[0].get(), env, fns, reason);
                    if (!reason.empty())
                        return Kind::Void;
                    if (a != Kind::I64) {
                        reason = "os.args_get() takes an i64 index";
                        return Kind::Void;
                    }
                    return Kind::Str;
                }
                if (c->callee == "string.len") {
                    if (c->args.size() != 1) {
                        reason = "string.len() takes exactly 1 argument";
                        return Kind::Void;
                    }
                    const Kind a = checkExpr(c->args[0].get(), env, fns, reason);
                    if (!reason.empty())
                        return Kind::Void;
                    if (a != Kind::Str) {
                        reason = "string.len() takes a string argument";
                        return Kind::Void;
                    }
                    return Kind::I64;
                }
                if (c->callee == "string.byte_at") {
                    if (c->args.size() != 2) {
                        reason = "string.byte_at() takes exactly 2 arguments (string, index)";
                        return Kind::Void;
                    }
                    const Kind a = checkExpr(c->args[0].get(), env, fns, reason);
                    if (!reason.empty())
                        return Kind::Void;
                    if (a != Kind::Str) {
                        reason = "string.byte_at() first argument must be a string";
                        return Kind::Void;
                    }
                    const Kind b = checkExpr(c->args[1].get(), env, fns, reason);
                    if (!reason.empty())
                        return Kind::Void;
                    if (b != Kind::I64) {
                        reason = "string.byte_at() second argument must be an i64 index";
                        return Kind::Void;
                    }
                    return Kind::I64;
                }
                reason = "call to '" + c->callee +
                         "' is outside the builtin native v0 subset (unknown function or "
                         "unsupported builtin; use --backend llvm)";
                return Kind::Void;
            }
            if (c->args.size() != it->second.params.size()) {
                reason = "call to '" + c->callee + "' has the wrong argument count";
                return Kind::Void;
            }
            return it->second.ret;
        }

        Kind checkExpr(const Expr* e,
                       const std::map<std::string, Kind>& env,
                       const std::map<std::string, FnSig>& fns,
                       std::string& reason) {
            if (dynamic_cast<const IntExpr*>(e))
                return Kind::I64;
            if (dynamic_cast<const BoolExpr*>(e))
                return Kind::Bool;
            if (dynamic_cast<const StringExpr*>(e))
                return Kind::Str;
            if (const auto* v = dynamic_cast<const VarExpr*>(e)) {
                const auto it = env.find(v->name);
                if (it == env.end()) {
                    reason = "variable '" + v->name + "' is outside the builtin native v0 subset";
                    return Kind::I64;
                }
                return it->second;
            }
            if (const auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                const Kind t = checkExpr(u->rhs.get(), env, fns, reason);
                if (!reason.empty())
                    return Kind::I64;
                if (u->op == TokenKind::Minus && t == Kind::I64)
                    return Kind::I64;
                if (u->op == TokenKind::Bang && t == Kind::Bool)
                    return Kind::Bool;
                reason = "this unary operator is outside the builtin native v0 subset";
                return Kind::I64;
            }
            if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                const Kind l = checkExpr(b->lhs.get(), env, fns, reason);
                if (!reason.empty())
                    return Kind::I64;
                const Kind r = checkExpr(b->rhs.get(), env, fns, reason);
                if (!reason.empty())
                    return Kind::I64;
                switch (b->op) {
                case TokenKind::Plus:
                    if (l == Kind::I64 && r == Kind::I64)
                        return Kind::I64;
                    if (l == Kind::Str && r == Kind::Str)
                        return Kind::Str;
                    break;
                case TokenKind::Minus:
                case TokenKind::Star:
                case TokenKind::Slash:
                case TokenKind::Percent:
                    if (l == Kind::I64 && r == Kind::I64)
                        return Kind::I64;
                    break;
                case TokenKind::EqualEqual:
                case TokenKind::BangEqual:
                    if (l == r && (l == Kind::I64 || l == Kind::Bool || l == Kind::Str))
                        return Kind::Bool;
                    break;
                case TokenKind::Less:
                case TokenKind::LessEqual:
                case TokenKind::Greater:
                case TokenKind::GreaterEqual:
                    if (l == Kind::I64 && r == Kind::I64)
                        return Kind::Bool;
                    break;
                case TokenKind::AndAnd:
                case TokenKind::OrOr:
                    if (l == Kind::Bool && r == Kind::Bool)
                        return Kind::Bool;
                    break;
                default:
                    break;
                }
                reason = "this operator/type combination is outside the builtin native v0 subset";
                return Kind::I64;
            }
            if (const auto* c = dynamic_cast<const CallExpr*>(e))
                return checkCall(c, env, fns, reason);
            reason = "this expression is outside the builtin native v0 subset "
                     "(f64, strings in variables, arrays, structs, pointers, and match values need "
                     "--backend llvm)";
            return Kind::I64;
        }

        void checkBlock(const Block* b,
                        std::map<std::string, Kind>& env,
                        const std::map<std::string, FnSig>& fns,
                        bool inLoop,
                        std::string& reason);

        void checkStmt(const Stmt* s,
                       std::map<std::string, Kind>& env,
                       const std::map<std::string, FnSig>& fns,
                       bool inLoop,
                       std::string& reason) {
            if (!reason.empty())
                return;
            if (const auto* nested = dynamic_cast<const Block*>(s)) {
                auto saved = env;
                checkBlock(nested, env, fns, inLoop, reason);
                env = std::move(saved);
                return;
            }
            if (const auto* l = dynamic_cast<const LetStmt*>(s)) {
                const Kind t = checkExpr(l->init.get(), env, fns, reason);
                if (!reason.empty())
                    return;
                if (t != Kind::I64 && t != Kind::Bool && t != Kind::Str) {
                    reason = "variable '" + l->name +
                             "' has a type outside the builtin native v0 subset (only i64, bool, "
                             "and string locals are supported)";
                    return;
                }
                env[l->name] = t;
                return;
            }
            if (const auto* a = dynamic_cast<const AssignStmt*>(s)) {
                if (!env.contains(a->name)) {
                    reason = "assignment to unknown variable '" + a->name + "'";
                    return;
                }
                checkExpr(a->value.get(), env, fns, reason);
                return;
            }
            if (dynamic_cast<const MemberAssignStmt*>(s) ||
                dynamic_cast<const IndexAssignStmt*>(s)) {
                reason = "struct/array assignment is outside the builtin native v0 subset";
                return;
            }
            if (const auto* e = dynamic_cast<const ExprStmt*>(s)) {
                checkExpr(e->expr.get(), env, fns, reason);
                return;
            }
            if (const auto* r = dynamic_cast<const ReturnStmt*>(s)) {
                if (r->expr)
                    checkExpr(r->expr.get(), env, fns, reason);
                return;
            }
            if (const auto* i = dynamic_cast<const IfStmt*>(s)) {
                const Kind t = checkExpr(i->cond.get(), env, fns, reason);
                if (!reason.empty())
                    return;
                if (t != Kind::Bool) {
                    reason = "if condition must be bool in the builtin native v0 subset";
                    return;
                }
                auto saved = env;
                checkBlock(i->thenBlock.get(), env, fns, inLoop, reason);
                env = saved;
                if (i->elseBlock) {
                    checkBlock(i->elseBlock.get(), env, fns, inLoop, reason);
                    env = saved;
                }
                return;
            }
            if (const auto* w = dynamic_cast<const WhileStmt*>(s)) {
                const Kind t = checkExpr(w->cond.get(), env, fns, reason);
                if (!reason.empty())
                    return;
                if (t != Kind::Bool) {
                    reason = "while condition must be bool in the builtin native v0 subset";
                    return;
                }
                auto saved = env;
                checkBlock(w->body.get(), env, fns, true, reason);
                env = saved;
                return;
            }
            if (dynamic_cast<const BreakStmt*>(s) || dynamic_cast<const ContinueStmt*>(s)) {
                if (!inLoop)
                    reason = "break/continue outside a loop";
                return;
            }
            reason = "this statement (match or unsupported form) is outside the builtin native v0 "
                     "subset";
        }

        void checkBlock(const Block* b,
                        std::map<std::string, Kind>& env,
                        const std::map<std::string, FnSig>& fns,
                        bool inLoop,
                        std::string& reason) {
            for (const auto& s : b->statements) {
                checkStmt(s.get(), env, fns, inLoop, reason);
                if (!reason.empty())
                    return;
            }
        }

        bool kindOfType(const Type& t, Kind& out) {
            if (t.kind == TypeKind::I64) {
                out = Kind::I64;
                return true;
            }
            if (t.kind == TypeKind::Bool) {
                out = Kind::Bool;
                return true;
            }
            if (t.kind == TypeKind::Void) {
                out = Kind::Void;
                return true;
            }
            if (t.kind == TypeKind::String) {
                out = Kind::Str;
                return true;
            }
            return false;
        }

        // ---------------------------------------------------------------------------
        // x86-64 emitter
        // ---------------------------------------------------------------------------

        class Emitter {
        public:
            explicit Emitter(CodeImage& image) : image_(image) {}

            std::size_t pos() const {
                return image_.bytes.size();
            }
            void def(const std::string& name) {
                image_.labels[name] = pos();
            }

            void b(std::uint8_t v) {
                image_.bytes.push_back(v);
            }
            void i32(std::uint32_t v) {
                for (int i = 0; i < 4; ++i)
                    image_.bytes.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
            }
            void i64(std::uint64_t v) {
                for (int i = 0; i < 8; ++i)
                    image_.bytes.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
            }

            // 32-bit relative fixup: field = symbol - (codeBase + at + 4) + addend.
            void rel(const std::string& symbol, std::int32_t addend = 0) {
                image_.fixups.push_back({pos(), symbol, addend});
                i32(0);
            }

            std::string label(const std::string& prefix) {
                return prefix + "$" + std::to_string(nextLabel_++);
            }

            void pushRbp() {
                b(0x55);
            }
            void pushRax() {
                b(0x50);
            }
            void pushRbx() {
                b(0x53);
            }
            void popRax() {
                b(0x58);
            }
            void popRcx() {
                b(0x59);
            }
            void popRbx() {
                b(0x5B);
            }
            void popRbp() {
                b(0x5D);
            }
            void movRbxRsp() {
                b(0x48);
                b(0x89);
                b(0xE3);
            }
            void movRspRbx() {
                b(0x48);
                b(0x89);
                b(0xDC);
            }
            void ret() {
                b(0xC3);
            }
            void ud2() {
                b(0x0F);
                b(0x0B);
            }
            void repMovsb() {
                b(0xF3);
                b(0xA4);
            }
            void movRbpRsp() {
                b(0x48);
                b(0x89);
                b(0xE5);
            }
            void movRspRbp() {
                b(0x48);
                b(0x89);
                b(0xEC);
            }
            void movR11Rsp() {
                b(0x49);
                b(0x89);
                b(0xE3);
            }
            void movRspR11() {
                b(0x4C);
                b(0x89);
                b(0xDC);
            }
            void subRsp(std::uint32_t n) {
                if (n <= 127) {
                    b(0x48);
                    b(0x83);
                    b(0xEC);
                    b(static_cast<std::uint8_t>(n));
                } else {
                    b(0x48);
                    b(0x81);
                    b(0xEC);
                    i32(n);
                }
            }
            void addRsp(std::uint32_t n) {
                if (n <= 127) {
                    b(0x48);
                    b(0x83);
                    b(0xC4);
                    b(static_cast<std::uint8_t>(n));
                } else {
                    b(0x48);
                    b(0x81);
                    b(0xC4);
                    i32(n);
                }
            }
            void andRspMinus16() {
                b(0x48);
                b(0x83);
                b(0xE4);
                b(0xF0);
            }
            void movRaxImm64(std::uint64_t v) {
                b(0x48);
                b(0xB8);
                i64(v);
            }
            void movEcxImm32(std::uint32_t v) {
                b(0xB9);
                i32(v);
            }
            void movEaxImm32(std::uint32_t v) {
                b(0xB8);
                i32(v);
            }
            void movEdxImm32(std::uint32_t v) {
                b(0xBA);
                i32(v);
            }
            void movEdiEax() {
                b(0x89);
                b(0xC7);
            }
            void movEcxEax() {
                b(0x89);
                b(0xC1);
            }
            void movRcxRax() {
                b(0x48);
                b(0x89);
                b(0xC1);
            }
            void movRdxRax() {
                b(0x48);
                b(0x89);
                b(0xC2);
            }
            void movRsiRcx() {
                b(0x48);
                b(0x89);
                b(0xCE);
            }
            void movRaxRdx() {
                b(0x48);
                b(0x89);
                b(0xD0);
            }
            void movRaxRdi() {
                b(0x48);
                b(0x89);
                b(0xF8);
            }
            void movRaxRsi() {
                b(0x48);
                b(0x89);
                b(0xF0);
            }
            void movRsiRax() {
                b(0x48);
                b(0x89);
                b(0xC6);
            }
            void movRsiRdx() {
                b(0x48);
                b(0x89);
                b(0xD6);
            }
            void movRaxR8() {
                b(0x4C);
                b(0x89);
                b(0xC0);
            }
            void movRdxRsi() {
                b(0x48);
                b(0x89);
                b(0xF2);
            }
            void movRcxRdx() {
                b(0x48);
                b(0x89);
                b(0xD1);
            }
            void movRcxRsi() {
                b(0x48);
                b(0x89);
                b(0xF1);
            }
            void movRcxR8() {
                b(0x4C);
                b(0x89);
                b(0xC1);
            }
            void movRcxR9() {
                b(0x4C);
                b(0x89);
                b(0xC9);
            }
            void movRcxR11() {
                b(0x4C);
                b(0x89);
                b(0xD9);
            }
            void movRdiRsi() {
                b(0x48);
                b(0x89);
                b(0xF7);
            }
            void movRsiR11() {
                b(0x4C);
                b(0x89);
                b(0xDE);
            }
            void movEaxFromRsp8(std::uint8_t d) {
                b(0x8B);
                b(0x44);
                b(0x24);
                b(d);
            }
            void movRsiImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC6);
                i32(v);
            }
            void movRdxImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC2);
                i32(v);
            }
            void movR8Imm32(std::uint32_t v) {
                b(0x49);
                b(0xC7);
                b(0xC0);
                i32(v);
            }
            void movR9Imm32(std::uint32_t v) {
                b(0x49);
                b(0xC7);
                b(0xC1);
                i32(v);
            }
            void movR10Imm32(std::uint32_t v) {
                b(0x41);
                b(0xBA);
                i32(v);
            }
            void movRcxImm10() {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(10);
            }
            void movR10Imm1() {
                b(0x41);
                b(0xBA);
                i32(1);
            }
            void xorEdx() {
                b(0x31);
                b(0xD2);
            }
            void xorEax() {
                b(0x31);
                b(0xC0);
            }
            void xorRax() {
                b(0x48);
                b(0x31);
                b(0xC0);
            }
            void xorEcx() {
                b(0x31);
                b(0xC9);
            }
            void xorEdi() {
                b(0x31);
                b(0xFF);
            }
            void xorR8d() {
                b(0x45);
                b(0x31);
                b(0xC0);
            }
            void xorR9d() {
                b(0x45);
                b(0x31);
                b(0xC9);
            }
            void xorR10d() {
                b(0x45);
                b(0x31);
                b(0xD2);
            }
            void addRaxImm32(std::uint32_t v) {
                b(0x48);
                b(0x05);
                i32(v);
            }
            void addRaxRcx() {
                b(0x48);
                b(0x01);
                b(0xC8);
            }
            void addRaxRdi() {
                b(0x48);
                b(0x01);
                b(0xF8);
            }
            void addRaxRsi() {
                b(0x48);
                b(0x01);
                b(0xF0);
            }
            void addRaxR9() {
                b(0x4C);
                b(0x01);
                b(0xC8);
            }
            void addRsiRdx() {
                b(0x48);
                b(0x01);
                b(0xD6);
            }
            void addRsiRcx() {
                b(0x48);
                b(0x01);
                b(0xCE);
            }
            void addR11Imm32(std::uint32_t v) {
                b(0x49);
                b(0x81);
                b(0xC3);
                i32(v);
            }
            void incRax() {
                b(0x48);
                b(0xFF);
                b(0xC0);
            }
            void incRcx() {
                b(0x48);
                b(0xFF);
                b(0xC1);
            }
            void incRdx() {
                b(0x48);
                b(0xFF);
                b(0xC2);
            }
            void incRsi() {
                b(0x48);
                b(0xFF);
                b(0xC6);
            }
            void incR8() {
                b(0x49);
                b(0xFF);
                b(0xC0);
            }
            void incR9() {
                b(0x49);
                b(0xFF);
                b(0xC1);
            }
            void decEcx() {
                b(0xFF);
                b(0xC9);
            }
            void decR8() {
                b(0x49);
                b(0xFF);
                b(0xC8);
            }
            void subRaxRcx() {
                b(0x48);
                b(0x29);
                b(0xC8);
            }
            void imulRaxRcx() {
                b(0x48);
                b(0x0F);
                b(0xAF);
                b(0xC1);
            }
            void cqo() {
                b(0x48);
                b(0x99);
            }
            void idivRcx() {
                b(0x48);
                b(0xF7);
                b(0xF9);
            }
            void divRcx() {
                b(0x48);
                b(0xF7);
                b(0xF1);
            }
            void negRax() {
                b(0x48);
                b(0xF7);
                b(0xD8);
            }
            void notBoolRax() {
                b(0x48);
                b(0x83);
                b(0xF0);
                b(0x01);
            }
            void andRaxRcx() {
                b(0x48);
                b(0x21);
                b(0xC8);
            }
            void orRaxRcx() {
                b(0x48);
                b(0x09);
                b(0xC8);
            }
            void cmpRaxRcx() {
                b(0x48);
                b(0x39);
                b(0xC8);
            }
            void cmpRcxRax() {
                b(0x48);
                b(0x39);
                b(0xC1);
            }
            void cmpRaxR11() {
                b(0x4C);
                b(0x39);
                b(0xD8);
            }
            void cmpRaxImm8(std::uint8_t v) {
                b(0x48);
                b(0x83);
                b(0xF8);
                b(v);
            }
            void cmpBytePtrRcx(std::uint8_t v) {
                b(0x80);
                b(0x39);
                b(v);
            }
            void cmpBytePtrRsi(std::uint8_t v) {
                b(0x80);
                b(0x3E);
                b(v);
            }
            void cmpAlDl() {
                b(0x38);
                b(0xC2);
            }
            void cmpAlImm10() {
                b(0x3C);
                b(0x0A);
            }
            void cmpAlImm13() {
                b(0x3C);
                b(0x0D);
            }
            void cmpRdxRax() {
                b(0x48);
                b(0x39);
                b(0xC2);
            }
            void testRaxRax() {
                b(0x48);
                b(0x85);
                b(0xC0);
            }
            void testRdxRdx() {
                b(0x48);
                b(0x85);
                b(0xD2);
            }
            void testRcxRcx() {
                b(0x48);
                b(0x85);
                b(0xC9);
            }
            void testEcxEcx() {
                b(0x85);
                b(0xC9);
            }
            void testAlAl() {
                b(0x84);
                b(0xC0);
            }
            void testR8R8() {
                b(0x4D);
                b(0x85);
                b(0xC0);
            }
            void testR10d() {
                b(0x45);
                b(0x85);
                b(0xD2);
            }
            void setcc(std::uint8_t cc) {
                b(0x0F);
                b(cc);
                b(0xC0);
            }
            void movzxEaxAl() {
                b(0x0F);
                b(0xB6);
                b(0xC0);
            }
            void addDl48() {
                b(0x80);
                b(0xC2);
                b(0x30);
            }
            void decRsi() {
                b(0x48);
                b(0xFF);
                b(0xCE);
            }
            void subRdxRsi() {
                b(0x48);
                b(0x29);
                b(0xF2);
            }
            void syscall() {
                b(0x0F);
                b(0x05);
            }
            void cld() {
                b(0xFC);
            }
            void repneScasb() {
                b(0xF2);
                b(0xAE);
            }
            void repeCmpsb() {
                b(0xF3);
                b(0xA6);
            }
            void notRcx() {
                b(0x48);
                b(0xF7);
                b(0xD1);
            }
            void decRcx() {
                b(0x48);
                b(0xFF);
                b(0xC9);
            }
            void movRdiRcx() {
                b(0x48);
                b(0x89);
                b(0xCF);
            }
            void movRcxImmNeg1() {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(0xFFFFFFFFu);
            }
            void storeByteRsiImm8(std::uint8_t v) {
                b(0xC6);
                b(0x06);
                b(v);
            }
            void storeByteRsiImm0() {
                b(0xC6);
                b(0x06);
                b(0x00);
            }
            void storeByteRdiImm0() {
                b(0xC6);
                b(0x07);
                b(0x00);
            }
            void storeDlRsi() {
                b(0x88);
                b(0x16);
            }
            void movAlFromRcx() {
                b(0x8A);
                b(0x01);
            }
            void movAlFromRsi() {
                b(0x8A);
                b(0x06);
            }
            void movAlFromRsiMinus1() {
                b(0x8A);
                b(0x46);
                b(0xFF);
            }
            void movDlFromRdx() {
                b(0x8A);
                b(0x12);
            }
            void movEdiImm32(std::uint32_t v) {
                b(0xBF);
                i32(v);
            }
            void leaR9FromRsp8(std::uint8_t d) {
                b(0x4C);
                b(0x8D);
                b(0x4C);
                b(0x24);
                b(d);
            }
            void storeQwordRsp32Imm0() {
                b(0x48);
                b(0xC7);
                b(0x44);
                b(0x24);
                b(0x20);
                i32(0);
            }
            void jmp(const std::string& target) {
                b(0xE9);
                rel(target);
            }
            void jcc(std::uint8_t cc, const std::string& target) {
                b(0x0F);
                b(cc);
                rel(target);
            }
            void call(const std::string& target) {
                b(0xE8);
                rel(target);
            }
            void callIAT(const std::string& importSym) {
                b(0xFF);
                b(0x15);
                rel(importSym);
            }
            void leaRcxRip(const std::string& s) {
                b(0x48);
                b(0x8D);
                b(0x0D);
                rel(s);
            }
            void leaRaxRip(const std::string& s) {
                b(0x48);
                b(0x8D);
                b(0x05);
                rel(s);
            }
            void leaR10Rip(const std::string& s) {
                b(0x4C);
                b(0x8D);
                b(0x15);
                rel(s);
            }
            // Arena word access: __nx_arena[0] = base, __nx_arena[1] = pos.
            void movR11FromR10() {
                b(0x4D);
                b(0x8B);
                b(0x1A);
            }
            void movR11FromR10Disp8(std::uint8_t d) {
                b(0x4D);
                b(0x8B);
                b(0x5A);
                b(d);
            }
            void movRdiFromR10Disp8(std::uint8_t d) {
                b(0x49);
                b(0x8B);
                b(0x7A);
                b(d);
            }
            void movR10FromRax() {
                b(0x49);
                b(0x89);
                b(0x02);
            }
            void movR10Disp8FromRax(std::uint8_t d) {
                b(0x49);
                b(0x89);
                b(0x42);
                b(d);
            }
            void movR10Disp8FromRsi(std::uint8_t d) {
                b(0x49);
                b(0x89);
                b(0x72);
                b(d);
            }
            void leaRdxFromRbp(std::int64_t off) {
                leaFromRbp(0x55, 0x95, off);
            }
            void leaRsiFromRbp(std::int64_t off) {
                leaFromRbp(0x75, 0xB5, off);
            }

            // rbp-relative loads/stores. Negative offsets address locals.
            void movRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x45, 0x85, off);
            }
            void subRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x2B, 0x45, 0x85, off);
            }
            void movRsiFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x75, 0xB5, off);
            }
            void movRbpFromRsi(std::int64_t off) {
                memToReg(0x48, 0x89, 0x75, 0xB5, off);
            }
            void movRbpFromRax(std::int64_t off) {
                memToReg(0x48, 0x89, 0x45, 0x85, off);
            }
            void movRcxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x4D, 0x8D, off);
            }
            void movRdxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x55, 0x95, off);
            }
            void movRdiFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x7D, 0xBD, off);
            }
            void movR8FromRbp(std::int64_t off) {
                memToReg(0x4C, 0x8B, 0x45, 0x85, off);
            }
            void movRcxToRbp(std::int64_t off) {
                memToReg(0x48, 0x89, 0x4D, 0x8D, off);
            }
            void movRdxToRbp(std::int64_t off) {
                memToReg(0x48, 0x89, 0x55, 0x95, off);
            }
            void
            movR11FromRbp(std::int64_t off) { // Positive offsets address incoming stack arguments.
                if (off >= 0) {
                    if (off <= 127) {
                        b(0x4C);
                        b(0x8B);
                        b(0x5D);
                        b(static_cast<std::uint8_t>(off));
                    } else {
                        b(0x4C);
                        b(0x8B);
                        b(0x9D);
                        i32(static_cast<std::uint32_t>(off));
                    }
                    return;
                }
                memToReg(0x4C, 0x8B, 0x5D, 0x9D, off);
            }
            void movRbpFromR11(std::int64_t off) {
                memToReg(0x4C, 0x89, 0x5D, 0x9D, off);
            }

        private:
            void memToReg(std::uint8_t rex,
                          std::uint8_t op,
                          std::uint8_t modrm8,
                          std::uint8_t modrm32,
                          std::int64_t off) {
                b(rex);
                b(op);
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }
            void leaFromRbp(std::uint8_t modrm8, std::uint8_t modrm32, std::int64_t off) {
                b(0x48);
                b(0x8D);
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }

            CodeImage& image_;
            std::size_t nextLabel_ = 0;
        };

        // Condition codes (0F-prefixed setcc/jcc second byte).
        constexpr std::uint8_t CC_E = 0x94, CC_NE = 0x95, CC_L = 0x9C, CC_LE = 0x9E;
        constexpr std::uint8_t CC_G = 0x9F, CC_GE = 0x9D;
        constexpr std::uint8_t J_E = 0x84, J_NE = 0x85, J_NS = 0x89;
        constexpr std::uint8_t J_L = 0x8C, J_AE = 0x83;
        constexpr std::uint8_t J_A = 0x87, J_LE = 0x8E;
        constexpr std::uint32_t kArenaSize = 0x1000000; // 16 MiB bump arena for strings

        // ---------------------------------------------------------------------------
        // Program code generator
        // ---------------------------------------------------------------------------

        class Generator {
        public:
            Generator(CodeImage& image, NativeTarget target)
                : image_(image), out_(image), target_(target) {}

            static Kind typeKind(const Type& t) {
                if (t.kind == TypeKind::Bool)
                    return Kind::Bool;
                if (t.kind == TypeKind::String)
                    return Kind::Str;
                if (t.kind == TypeKind::Void)
                    return Kind::Void;
                return Kind::I64;
            }

            void generate(const Program& program) {
                for (const auto& f : program.functions) {
                    FnSig sig;
                    for (const auto& p : f.params)
                        sig.params.push_back(typeKind(p.type));
                    sig.ret = typeKind(f.ret);
                    sigs_[f.name] = sig;
                }
                for (const auto& f : program.functions)
                    genFunction(f);
                genWrite();
                genWriteCstr();
                genStrEq();
                genStringLen();
                genStringByteAt();
                genStrCat();
                genStrI64();
                genInput();
                genOom();
                genPrintI64();
                genPrintBool();
                genEntry();
            }

        private:
            CodeImage& image_;
            Emitter out_;
            NativeTarget target_;
            std::map<std::string, FnSig> sigs_;
            std::map<std::string, std::size_t> slots_;
            std::size_t slotCount_ = 0;
            std::string endLabel_;
            std::vector<std::string> breakStack_;
            std::vector<std::string> continueStack_;
            const Function* currentFn_ = nullptr;

            static std::int64_t slotOff(std::size_t slot) {
                return -static_cast<std::int64_t>(8 * (slot + 1));
            }

            std::size_t internString(const std::string& text) {
                for (std::size_t i = 0; i < image_.strings.size(); ++i)
                    if (image_.strings[i] == text)
                        return i;
                image_.strings.push_back(text);
                return image_.strings.size() - 1;
            }

            void genFunction(const Function& f) {
                currentFn_ = &f;
                slots_.clear();
                slotCount_ = 0;
                for (const auto& p : f.params)
                    slots_[p.name] = slotCount_++;
                // Pre-scan let bindings so every local owns a fixed frame slot.
                collectLocals(f.body.get());
                const std::size_t frame = (slotCount_ * 8 + 15) / 16 * 16;

                out_.def(f.name);
                out_.pushRbp();
                out_.movRbpRsp();
                if (frame > 0)
                    out_.subRsp(static_cast<std::uint32_t>(frame));
                // Arguments arrive on the stack, pushed left-to-right, so argument k
                // sits at rbp+16+8*(nargs-1-k) once the frame is established.
                const std::size_t nargs = f.params.size();
                for (std::size_t k = 0; k < nargs; ++k) {
                    const std::int64_t src = 16 + static_cast<std::int64_t>(8 * (nargs - 1 - k));
                    out_.movR11FromRbp(src);
                    out_.movRbpFromR11(slotOff(slots_[f.params[k].name]));
                }
                endLabel_ = out_.label(f.name + "$end");
                breakStack_.clear();
                continueStack_.clear();
                genBlock(f.body.get());
                out_.def(endLabel_);
                if (f.name == "main" && f.ret.kind == TypeKind::Void)
                    out_.xorEax();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
                currentFn_ = nullptr;
            }

            void collectLocals(const Block* b) {
                for (const auto& s : b->statements) {
                    if (const auto* nested = dynamic_cast<const Block*>(s.get())) {
                        collectLocals(nested);
                    } else if (const auto* l = dynamic_cast<const LetStmt*>(s.get())) {
                        if (!slots_.contains(l->name))
                            slots_[l->name] = slotCount_++;
                    } else if (const auto* i = dynamic_cast<const IfStmt*>(s.get())) {
                        collectLocals(i->thenBlock.get());
                        if (i->elseBlock)
                            collectLocals(i->elseBlock.get());
                    } else if (const auto* w = dynamic_cast<const WhileStmt*>(s.get())) {
                        collectLocals(w->body.get());
                    }
                }
            }

            void genBlock(const Block* b) {
                for (const auto& s : b->statements)
                    genStmt(s.get());
            }

            void genStmt(const Stmt* s) {
                if (const auto* nested = dynamic_cast<const Block*>(s)) {
                    genBlock(nested);
                    return;
                }
                if (const auto* l = dynamic_cast<const LetStmt*>(s)) {
                    genExpr(l->init.get());
                    out_.movRbpFromRax(slotOff(slots_.at(l->name)));
                    return;
                }
                if (const auto* a = dynamic_cast<const AssignStmt*>(s)) {
                    genExpr(a->value.get());
                    out_.movRbpFromRax(slotOff(slots_.at(a->name)));
                    return;
                }
                if (const auto* e = dynamic_cast<const ExprStmt*>(s)) {
                    genExpr(e->expr.get());
                    return;
                }
                if (const auto* r = dynamic_cast<const ReturnStmt*>(s)) {
                    if (r->expr)
                        genExpr(r->expr.get());
                    out_.jmp(endLabel_);
                    return;
                }
                if (const auto* i = dynamic_cast<const IfStmt*>(s)) {
                    genExpr(i->cond.get());
                    out_.testRaxRax();
                    const std::string els = out_.label("if.else");
                    const std::string end = out_.label("if.end");
                    out_.jcc(J_E, i->elseBlock ? els : end);
                    genBlock(i->thenBlock.get());
                    if (i->elseBlock) {
                        out_.jmp(end);
                        out_.def(els);
                        genBlock(i->elseBlock.get());
                    }
                    out_.def(end);
                    return;
                }
                if (const auto* w = dynamic_cast<const WhileStmt*>(s)) {
                    const std::string cond = out_.label("while.cond");
                    const std::string body = out_.label("while.body");
                    const std::string end = out_.label("while.end");
                    out_.jmp(cond);
                    out_.def(cond);
                    genExpr(w->cond.get());
                    out_.testRaxRax();
                    out_.jcc(J_E, end);
                    out_.def(body);
                    breakStack_.push_back(end);
                    continueStack_.push_back(cond);
                    genBlock(w->body.get());
                    breakStack_.pop_back();
                    continueStack_.pop_back();
                    out_.jmp(cond);
                    out_.def(end);
                    return;
                }
                if (dynamic_cast<const BreakStmt*>(s)) {
                    out_.jmp(breakStack_.back());
                    return;
                }
                if (dynamic_cast<const ContinueStmt*>(s)) {
                    out_.jmp(continueStack_.back());
                    return;
                }
                throw std::runtime_error(
                    "builtin native v0: unsupported statement reached codegen");
            }

            void genExpr(const Expr* e) {
                if (const auto* x = dynamic_cast<const IntExpr*>(e)) {
                    out_.movRaxImm64(static_cast<std::uint64_t>(x->value));
                    return;
                }
                if (const auto* x = dynamic_cast<const BoolExpr*>(e)) {
                    out_.movRaxImm64(x->value ? 1 : 0);
                    return;
                }
                if (const auto* x = dynamic_cast<const VarExpr*>(e)) {
                    out_.movRaxFromRbp(slotOff(slots_.at(x->name)));
                    return;
                }
                if (dynamic_cast<const StringExpr*>(e)) {
                    const auto* s = static_cast<const StringExpr*>(e);
                    const std::size_t id = internString(s->value);
                    out_.leaRaxRip("str" + std::to_string(id));
                    return;
                }
                if (const auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                    genExpr(u->rhs.get());
                    if (u->op == TokenKind::Minus)
                        out_.negRax();
                    else
                        out_.notBoolRax();
                    return;
                }
                if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                    // String concatenation and comparison go through arena helpers.
                    if (b->op == TokenKind::Plus && exprIsString(b->lhs.get())) {
                        genExpr(b->lhs.get());
                        out_.pushRax();
                        genExpr(b->rhs.get());
                        out_.movRdxRax();
                        out_.popRcx();
                        out_.call("__nx_str_cat");
                        return;
                    }
                    if ((b->op == TokenKind::EqualEqual || b->op == TokenKind::BangEqual) &&
                        exprIsString(b->lhs.get())) {
                        genExpr(b->lhs.get());
                        out_.pushRax();
                        genExpr(b->rhs.get());
                        out_.movRdxRax();
                        out_.popRcx();
                        out_.call("__nx_str_eq");
                        if (b->op == TokenKind::BangEqual)
                            out_.notBoolRax();
                        return;
                    }
                    genExpr(b->lhs.get());
                    out_.pushRax();
                    genExpr(b->rhs.get());
                    out_.movRcxRax();
                    out_.popRax();
                    switch (b->op) {
                    case TokenKind::Plus:
                        out_.addRaxRcx();
                        break;
                    case TokenKind::Minus:
                        out_.subRaxRcx();
                        break;
                    case TokenKind::Star:
                        out_.imulRaxRcx();
                        break;
                    case TokenKind::Slash:
                        out_.cqo();
                        out_.idivRcx();
                        break;
                    case TokenKind::Percent:
                        out_.cqo();
                        out_.idivRcx();
                        out_.movRaxRdx();
                        break;
                    case TokenKind::EqualEqual:
                        out_.cmpRaxRcx();
                        out_.setcc(CC_E);
                        out_.movzxEaxAl();
                        break;
                    case TokenKind::BangEqual:
                        out_.cmpRaxRcx();
                        out_.setcc(CC_NE);
                        out_.movzxEaxAl();
                        break;
                    case TokenKind::Less:
                        out_.cmpRaxRcx();
                        out_.setcc(CC_L);
                        out_.movzxEaxAl();
                        break;
                    case TokenKind::LessEqual:
                        out_.cmpRaxRcx();
                        out_.setcc(CC_LE);
                        out_.movzxEaxAl();
                        break;
                    case TokenKind::Greater:
                        out_.cmpRaxRcx();
                        out_.setcc(CC_G);
                        out_.movzxEaxAl();
                        break;
                    case TokenKind::GreaterEqual:
                        out_.cmpRaxRcx();
                        out_.setcc(CC_GE);
                        out_.movzxEaxAl();
                        break;
                    case TokenKind::AndAnd:
                        out_.andRaxRcx();
                        break;
                    case TokenKind::OrOr:
                        out_.orRaxRcx();
                        break;
                    default:
                        throw std::runtime_error(
                            "builtin native v0: unsupported operator reached codegen");
                    }
                    return;
                }
                if (const auto* c = dynamic_cast<const CallExpr*>(e)) {
                    if (c->callee == "print") {
                        genPrint(c->args[0].get());
                        return;
                    }
                    if (c->callee == "input" || c->callee == "str_i64") {
                        genExpr(c->args[0].get());
                        out_.call(c->callee == "input" ? "__nx_input" : "__nx_str_i64");
                        return;
                    }
                    if (c->callee == "os.args_count") {
                        out_.call("__nx_os_args_count");
                        return;
                    }
                    if (c->callee == "os.args_get") {
                        genExpr(c->args[0].get());
                        out_.call("__nx_os_args_get");
                        return;
                    }
                    if (c->callee == "string.len") {
                        genExpr(c->args[0].get());
                        out_.call("__nx_string_len");
                        return;
                    }
                    if (c->callee == "string.byte_at") {
                        genExpr(c->args[0].get());
                        out_.pushRax();
                        genExpr(c->args[1].get());
                        out_.movRcxRax();
                        out_.popRax();
                        out_.call("__nx_string_byte_at");
                        return;
                    }
                    for (const auto& a : c->args) {
                        genExpr(a.get());
                        out_.pushRax();
                    }
                    out_.call(c->callee);
                    if (!c->args.empty())
                        out_.addRsp(static_cast<std::uint32_t>(8 * c->args.size()));
                    return;
                }
                throw std::runtime_error(
                    "builtin native v0: unsupported expression reached codegen");
            }

            void genPrint(const Expr* arg) {
                if (const auto* s = dynamic_cast<const StringExpr*>(arg)) {
                    // Fast path: the literal plus its newline is emitted once.
                    const std::string withNewline = s->value + "\n";
                    const std::size_t id = internString(withNewline);
                    out_.leaRcxRip("str" + std::to_string(id));
                    out_.movRdxImm32(static_cast<std::uint32_t>(withNewline.size()));
                    out_.call("__nx_write");
                    return;
                }
                genExpr(arg);
                if (exprIsString(arg))
                    out_.call("__nx_write_cstr");
                else if (exprIsBool(arg))
                    out_.call("__nx_print_bool");
                else
                    out_.call("__nx_print_i64");
            }

            // True for string-typed expressions (literals, string vars/params, string
            // concatenation, and the input/str_i64 builtins).
            bool exprIsString(const Expr* e) {
                if (dynamic_cast<const StringExpr*>(e))
                    return true;
                if (dynamic_cast<const IntExpr*>(e) || dynamic_cast<const BoolExpr*>(e))
                    return false;
                if (const auto* v = dynamic_cast<const VarExpr*>(e))
                    return localKind(v->name) == Kind::Str;
                if (const auto* b = dynamic_cast<const BinaryExpr*>(e))
                    return b->op == TokenKind::Plus && exprIsString(b->lhs.get()) &&
                           exprIsString(b->rhs.get());
                if (const auto* c = dynamic_cast<const CallExpr*>(e)) {
                    if (c->callee == "input" || c->callee == "str_i64")
                        return true;
                    if (c->callee == "print")
                        return false;
                    const auto it = sigs_.find(c->callee);
                    return it != sigs_.end() && it->second.ret == Kind::Str;
                }
                return false;
            }

            // Mirrors the checker's kind rules to select the right print helper.
            bool exprIsBool(const Expr* e) {
                if (dynamic_cast<const BoolExpr*>(e))
                    return true;
                if (dynamic_cast<const IntExpr*>(e))
                    return false;
                if (dynamic_cast<const StringExpr*>(e))
                    return false;
                if (const auto* v = dynamic_cast<const VarExpr*>(e))
                    return localKind(v->name) == Kind::Bool;
                if (const auto* u = dynamic_cast<const UnaryExpr*>(e))
                    return u->op == TokenKind::Bang;
                if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                    switch (b->op) {
                    case TokenKind::EqualEqual:
                    case TokenKind::BangEqual:
                    case TokenKind::Less:
                    case TokenKind::LessEqual:
                    case TokenKind::Greater:
                    case TokenKind::GreaterEqual:
                    case TokenKind::AndAnd:
                    case TokenKind::OrOr:
                        return true;
                    default:
                        return false;
                    }
                }
                if (const auto* c = dynamic_cast<const CallExpr*>(e)) {
                    if (c->callee == "print")
                        return false;
                    const auto it = sigs_.find(c->callee);
                    return it != sigs_.end() && it->second.ret == Kind::Bool;
                }
                return false;
            }

            Kind localKind(const std::string& name) {
                if (currentFn_) {
                    for (const auto& p : currentFn_->params)
                        if (p.name == name)
                            return typeKind(p.type);
                    Kind found = Kind::I64;
                    bool ok = false;
                    findLet(currentFn_->body.get(), name, found, ok);
                    if (ok)
                        return found;
                }
                return Kind::I64;
            }

            void findLet(const Block* b, const std::string& name, Kind& found, bool& ok) {
                for (const auto& s : b->statements) {
                    if (ok)
                        return;
                    if (const auto* nested = dynamic_cast<const Block*>(s.get())) {
                        findLet(nested, name, found, ok);
                    } else if (const auto* l = dynamic_cast<const LetStmt*>(s.get())) {
                        if (l->name == name) {
                            found = initKind(l->init.get());
                            ok = true;
                        }
                    } else if (const auto* i = dynamic_cast<const IfStmt*>(s.get())) {
                        findLet(i->thenBlock.get(), name, found, ok);
                        if (!ok && i->elseBlock)
                            findLet(i->elseBlock.get(), name, found, ok);
                    } else if (const auto* w = dynamic_cast<const WhileStmt*>(s.get())) {
                        findLet(w->body.get(), name, found, ok);
                    }
                }
            }

            Kind initKind(const Expr* e) {
                if (dynamic_cast<const BoolExpr*>(e))
                    return Kind::Bool;
                if (dynamic_cast<const StringExpr*>(e))
                    return Kind::Str;
                if (const auto* u = dynamic_cast<const UnaryExpr*>(e))
                    return u->op == TokenKind::Bang ? Kind::Bool : Kind::I64;
                if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                    switch (b->op) {
                    case TokenKind::EqualEqual:
                    case TokenKind::BangEqual:
                    case TokenKind::Less:
                    case TokenKind::LessEqual:
                    case TokenKind::Greater:
                    case TokenKind::GreaterEqual:
                    case TokenKind::AndAnd:
                    case TokenKind::OrOr:
                        return Kind::Bool;
                    default:
                        return Kind::I64;
                    }
                }
                if (const auto* c = dynamic_cast<const CallExpr*>(e)) {
                    if (c->callee == "input" || c->callee == "str_i64")
                        return Kind::Str;
                    const auto it = sigs_.find(c->callee);
                    if (it != sigs_.end())
                        return it->second.ret;
                }
                if (const auto* v = dynamic_cast<const VarExpr*>(e))
                    return localKind(v->name);
                return Kind::I64;
            }

            // Call the shared write helper. r10/r11 do NOT survive OS calls
            // (the syscall instruction itself clobbers rcx+r11, and ntdll
            // trampolines clobber r10), so the stack pointer is sheltered in the
            // callee-saved rbx instead. Nothing else in the stubs uses rbx.
            void stubCall(const std::string& target) {
                if (target_ == NativeTarget::WindowsX64) {
                    out_.pushRbx();
                    out_.movRbxRsp();
                    out_.andRspMinus16();
                    out_.subRsp(32);
                    out_.call(target);
                    out_.movRspRbx();
                    out_.popRbx();
                } else {
                    out_.call(target);
                }
            }

            // __nx_write: rcx = pointer, rdx = length. Writes to stdout.
            void genWrite() {
                out_.def("__nx_write");
                if (target_ == NativeTarget::WindowsX64) {
                    out_.pushRbp();
                    out_.movRbpRsp();
                    out_.subRsp(96);
                    out_.andRspMinus16();
                    out_.movRcxToRbp(-8);
                    out_.movRdxToRbp(-16);
                    // handle = GetStdHandle(STD_OUTPUT_HANDLE = -11)
                    out_.movEcxImm32(static_cast<std::uint32_t>(-11));
                    out_.subRsp(32);
                    out_.callIAT("__iat_GetStdHandle");
                    out_.addRsp(32);
                    out_.movRbpFromRax(-24);
                    // WriteFile(handle, ptr, len, NULL, NULL)
                    out_.movRcxFromRbp(-24);
                    out_.movRdxFromRbp(-8);
                    out_.movR8FromRbp(-16);
                    out_.xorR9d();
                    out_.subRsp(48);
                    out_.storeQwordRsp32Imm0();
                    out_.callIAT("__iat_WriteFile");
                    out_.addRsp(48);
                    out_.movRspRbp();
                    out_.popRbp();
                    out_.ret();
                } else {
                    out_.pushRbp();
                    out_.movRbpRsp();
                    out_.movRsiRcx();
                    out_.movEaxImm32(1); // sys_write
                    out_.movEdiEax();    // fd = 1 (stdout)
                    out_.syscall();
                    out_.movRspRbp();
                    out_.popRbp();
                    out_.ret();
                }
            }

            // __nx_print_i64: rax = value. Prints decimal + newline.
            void genPrintI64() {
                out_.def("__nx_print_i64");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.andRspMinus16();
                out_.leaRsiFromRbp(-48);
                out_.decRsi();
                out_.storeByteRsiImm8(0x0A);
                out_.xorR10d();
                out_.testRaxRax();
                const std::string positive = out_.label("i64.pos");
                out_.jcc(J_NS, positive);
                out_.negRax();
                out_.movR10Imm1();
                out_.def(positive);
                out_.movRcxImm10();
                const std::string digit = out_.label("i64.digit");
                out_.def(digit);
                out_.xorEdx();
                out_.divRcx();
                out_.addDl48();
                out_.decRsi();
                out_.storeDlRsi();
                out_.testRaxRax();
                out_.jcc(J_NE, digit);
                out_.testR10d();
                const std::string noSign = out_.label("i64.nosign");
                out_.jcc(J_E, noSign);
                out_.decRsi();
                out_.storeByteRsiImm8(0x2D);
                out_.def(noSign);
                out_.leaRdxFromRbp(-48);
                out_.subRdxRsi();
                out_.movRcxRsi();
                stubCall("__nx_write");
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }

            // __nx_print_bool: rax = 0/1. Prints "true"/"false" + newline.
            void genPrintBool() {
                const std::size_t t = internString("true\n");
                const std::size_t f = internString("false\n");
                out_.def("__nx_print_bool");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.testRaxRax();
                const std::string isFalse = out_.label("bool.false");
                const std::string go = out_.label("bool.go");
                out_.jcc(J_E, isFalse);
                out_.leaRcxRip("str" + std::to_string(t));
                out_.movEdxImm32(5);
                out_.jmp(go);
                out_.def(isFalse);
                out_.leaRcxRip("str" + std::to_string(f));
                out_.movEdxImm32(6);
                out_.def(go);
                stubCall("__nx_write");
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }

            // __nx_write_cstr: rax = NUL-terminated string. Writes it + newline.
            void genWriteCstr() {
                const std::size_t nl = internString("\n");
                out_.def("__nx_write_cstr");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.movRcxRax();
                out_.movRsiRcx();
                out_.xorEdx();
                const std::string loop = out_.label("cstr.len");
                const std::string done = out_.label("cstr.done");
                out_.def(loop);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, done);
                out_.incRsi();
                out_.incRdx();
                out_.jmp(loop);
                out_.def(done);
                stubCall("__nx_write");
                out_.leaRcxRip("str" + std::to_string(nl));
                out_.movEdxImm32(1);
                stubCall("__nx_write");
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }

            // __nx_str_eq: rcx = a, rdx = b (NUL-terminated). Returns 0/1 in rax.
            // Implemented with repne/repe string instructions over strlen(a)+1 bytes
            // (the trailing NUL decides prefix cases correctly).
            void genStrEq() {
                out_.def("__nx_str_eq");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(32);
                out_.movRcxToRbp(-8);
                out_.movRdxToRbp(-16);
                out_.cld();
                out_.movRdiRcx();
                out_.movRcxImmNeg1();
                out_.xorEax();
                out_.repneScasb();
                out_.notRcx();
                out_.decRcx(); // rcx = strlen(a)
                out_.incRcx(); // include the NUL terminator
                out_.movRsiFromRbp(-8);
                out_.movRdiFromRbp(-16);
                out_.repeCmpsb();
                out_.setcc(CC_E);
                out_.movzxEaxAl();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }

            // __nx_string_len: rax = NUL-terminated string. Returns byte length in rax.
            void genStringLen() {
                out_.def("__nx_string_len");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.movRcxRax();
                out_.xorRax();
                const std::string loop = out_.label("strlen.loop");
                const std::string done = out_.label("strlen.done");
                out_.def(loop);
                out_.cmpBytePtrRcx(0);
                out_.jcc(J_E, done);
                out_.incRcx();
                out_.incRax();
                out_.jmp(loop);
                out_.def(done);
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }

            // __nx_string_byte_at: rax = string, rcx = index. Returns byte (0-255) in rax, or -1 if
            // out of bounds.
            void genStringByteAt() {
                out_.def("__nx_string_byte_at");
                out_.pushRbp();
                out_.movRbpRsp();
                // rax = string, rcx = index
                // Save string pointer in rdx
                out_.movRdxRax();
                // Check if index < 0 (signed comparison)
                out_.testRcxRcx();
                const std::string neg = out_.label("sba.neg");
                out_.jcc(J_L, neg);
                // Get string length
                out_.movRsiRdx(); // string -> rsi
                out_.xorRax();    // length counter
                const std::string lenLoop = out_.label("sba.len");
                const std::string lenDone = out_.label("sba.lendone");
                out_.def(lenLoop);
                out_.cmpBytePtrRsi(0);
                out_.jcc(J_E, lenDone);
                out_.incRsi();
                out_.incRax();
                out_.jmp(lenLoop);
                out_.def(lenDone);
                // Compare index (rcx) with length (rax)
                out_.cmpRcxRax();
                out_.jcc(J_AE, neg); // index >= length -> out of bounds
                // In bounds: load byte at index (string pointer in rdx)
                out_.movRsiRdx(); // string -> rsi
                out_.addRsiRcx(); // rsi += index
                out_.movAlFromRsi();
                out_.movzxEaxAl();
                const std::string ret = out_.label("sba.ret");
                out_.jmp(ret);
                out_.def(neg);
                out_.movRaxImm64(0xFFFFFFFFFFFFFFFFu); // -1
                out_.def(ret);
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }

            // __nx_str_cat: rcx = a, rdx = b. Returns a fresh arena string in rax.
            void genStrCat() {
                out_.def("__nx_str_cat");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.movRcxToRbp(-8);
                out_.movRdxToRbp(-16);
                out_.movRsiFromRbp(-8);
                out_.xorR8d();
                const std::string lenA = out_.label("cat.lenA");
                const std::string gotA = out_.label("cat.gotA");
                const std::string lenB = out_.label("cat.lenB");
                const std::string gotB = out_.label("cat.gotB");
                const std::string oom = out_.label("cat.oom");
                out_.def(lenA);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, gotA);
                out_.incRsi();
                out_.incR8();
                out_.jmp(lenA);
                out_.def(gotA);
                out_.movRsiFromRbp(-16);
                out_.xorR9d();
                out_.def(lenB);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, gotB);
                out_.incRsi();
                out_.incR9();
                out_.jmp(lenB);
                out_.def(gotB);
                out_.movRaxR8();
                out_.addRaxR9();
                out_.incRax();
                out_.leaR10Rip("__nx_arena");
                out_.movR11FromR10Disp8(8);
                out_.movRsiR11();
                out_.addRaxRsi();
                out_.movR11FromR10();
                out_.addR11Imm32(kArenaSize);
                out_.cmpRaxR11();
                out_.jcc(J_A, oom);
                out_.movR10Disp8FromRax(8);
                out_.movRaxRsi();
                out_.movRdiRsi();
                out_.movRsiFromRbp(-8);
                out_.movRcxR8();
                out_.repMovsb();
                out_.movRsiFromRbp(-16);
                out_.movRcxR9();
                out_.repMovsb();
                out_.storeByteRdiImm0();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
                out_.def(oom);
                out_.jmp("__nx_oom");
            }

            // __nx_str_i64: rax = value. Returns a fresh arena string in rax.
            void genStrI64() {
                out_.def("__nx_str_i64");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.andRspMinus16();
                out_.leaRsiFromRbp(-32);
                out_.xorR10d();
                out_.testRaxRax();
                const std::string positive = out_.label("s64.pos");
                out_.jcc(J_NS, positive);
                out_.negRax();
                out_.movR10Imm1();
                out_.def(positive);
                out_.movRcxImm10();
                const std::string digit = out_.label("s64.digit");
                const std::string noSign = out_.label("s64.nosign");
                const std::string oom = out_.label("s64.oom");
                out_.def(digit);
                out_.xorEdx();
                out_.divRcx();
                out_.addDl48();
                out_.decRsi();
                out_.storeDlRsi();
                out_.testRaxRax();
                out_.jcc(J_NE, digit);
                out_.testR10d();
                out_.jcc(J_E, noSign);
                out_.decRsi();
                out_.storeByteRsiImm8(0x2D);
                out_.def(noSign);
                out_.leaRdxFromRbp(-32);
                out_.subRdxRsi();
                out_.leaR10Rip("__nx_arena");
                out_.movRdiFromR10Disp8(8);
                out_.movRaxRdx();
                out_.incRax();
                out_.addRaxRdi();
                out_.movR11FromR10();
                out_.addR11Imm32(kArenaSize);
                out_.cmpRaxR11();
                out_.jcc(J_A, oom);
                out_.movR10Disp8FromRax(8);
                out_.movRaxRdi();
                out_.movRcxRdx();
                out_.repMovsb();
                out_.storeByteRdiImm0();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
                out_.def(oom);
                out_.jmp("__nx_oom");
            }

            // __nx_input: rax = prompt (NUL-terminated). Prints it, reads one line
            // from stdin, and returns a fresh arena string without the newline.
            void genInput() {
                out_.def("__nx_input");
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.andRspMinus16();
                out_.movRcxRax();
                out_.movRsiRcx();
                out_.xorEdx();
                const std::string plen = out_.label("in.plen");
                const std::string pdone = out_.label("in.pdone");
                out_.def(plen);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, pdone);
                out_.incRsi();
                out_.incRdx();
                out_.jmp(plen);
                out_.def(pdone);
                stubCall("__nx_write");
                // rsi = arena cursor (callee-saved, survives calls); [rbp-8] = start.
                // r10 (arena base) is reloaded at the top of every iteration because
                // OS calls destroy volatile registers.
                out_.leaR10Rip("__nx_arena");
                out_.movR11FromR10Disp8(8);
                out_.movRsiR11();
                out_.movRbpFromRsi(-8);
                if (target_ == NativeTarget::WindowsX64) {
                    out_.movEcxImm32(static_cast<std::uint32_t>(-10)); // STD_INPUT_HANDLE
                    out_.subRsp(32);
                    out_.callIAT("__iat_GetStdHandle");
                    out_.addRsp(32);
                    out_.movRbpFromRax(-16);
                }
                const std::string readLoop = out_.label("in.read");
                const std::string scan = out_.label("in.scan");
                const std::string chunkDone = out_.label("in.chunk");
                const std::string foundNl = out_.label("in.nl");
                const std::string eof = out_.label("in.eof");
                const std::string oom = out_.label("in.oom");
                out_.def(readLoop);
                out_.leaR10Rip("__nx_arena");
                out_.movRaxRsi();
                out_.addRaxImm32(4097);
                out_.movR11FromR10();
                out_.addR11Imm32(kArenaSize);
                out_.cmpRaxR11();
                out_.jcc(J_A, oom);
                if (target_ == NativeTarget::WindowsX64) {
                    out_.movRcxFromRbp(-16);
                    out_.movRdxRsi();
                    out_.movR8Imm32(4096);
                    out_.subRsp(64);
                    out_.storeQwordRsp32Imm0();
                    out_.leaR9FromRsp8(0x28);
                    out_.callIAT("__iat_ReadFile");
                    out_.movEaxFromRsp8(0x28);
                    out_.addRsp(64);
                    out_.testRaxRax();
                    out_.jcc(J_E, eof);
                    out_.movEcxEax();
                } else {
                    out_.movEaxImm32(0); // sys_read
                    out_.xorEdi();       // fd 0
                    out_.movRdxImm32(4096);
                    out_.syscall();
                    out_.testRaxRax();
                    out_.jcc(J_LE, eof);
                    out_.movEcxEax();
                }
                out_.def(scan);
                out_.testEcxEcx();
                out_.jcc(J_E, chunkDone);
                out_.movAlFromRsi();
                out_.cmpAlImm10();
                out_.jcc(J_E, foundNl);
                out_.incRsi();
                out_.decEcx();
                out_.jmp(scan);
                out_.def(chunkDone);
                out_.jmp(readLoop);
                out_.def(foundNl);
                // total = cursor - start; drop a preceding '\r' (Windows console).
                out_.movRaxRsi();
                out_.subRaxFromRbp(-8);
                out_.testRaxRax();
                out_.jcc(J_E, eof);
                out_.movAlFromRsiMinus1();
                out_.cmpAlImm13();
                out_.jcc(J_NE, eof);
                out_.decRsi();
                out_.def(eof);
                out_.storeByteRsiImm0();
                out_.incRsi();
                out_.leaR10Rip("__nx_arena"); // r10 died in the read call
                out_.movR10Disp8FromRsi(8);
                out_.movRaxFromRbp(-8);
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
                out_.def(oom);
                out_.jmp("__nx_oom");
            }

            // __nx_oom: clean out-of-string-memory failure (stderr + exit 1).
            void genOom() {
                const std::size_t m = internString("NEXUS runtime error: out of string memory\n");
                out_.def("__nx_oom");
                out_.leaRcxRip("str" + std::to_string(m));
                out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
                if (target_ == NativeTarget::WindowsX64) {
                    out_.andRspMinus16();
                    stubCall("__nx_write");
                    out_.andRspMinus16();
                    out_.movEcxImm32(1);
                    out_.subRsp(32);
                    out_.callIAT("__iat_ExitProcess");
                    out_.ud2();
                } else {
                    out_.movRsiRcx();
                    out_.movEaxImm32(1); // sys_write
                    out_.movEdiImm32(2); // stderr
                    out_.syscall();
                    out_.movEaxImm32(60); // sys_exit
                    out_.movEdiImm32(1);
                    out_.syscall();
                }
            }

            void genEntry() {
                out_.def("_start");
                // Reserve the string bump arena before user code runs: 16 MiB of
                // committed/mapped memory, tracked in the __nx_arena words.
                const std::string noArena = out_.label("start.noarena");
                if (target_ == NativeTarget::WindowsX64) {
                    // VirtualAlloc(NULL, 16 MiB, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE).
                    // OS entry rsp is 8 (mod 16): reserve 8 bytes padding + 32 shadow.
                    out_.xorEcx();
                    out_.movRdxImm32(kArenaSize);
                    out_.movR8Imm32(0x3000);
                    out_.movR9Imm32(0x04);
                    out_.subRsp(40);
                    out_.callIAT("__iat_VirtualAlloc");
                    out_.addRsp(40);
                    out_.testRaxRax();
                    out_.jcc(J_E, noArena);
                } else {
                    // mmap(NULL, 16 MiB, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0).
                    out_.xorEdi();
                    out_.movRsiImm32(kArenaSize);
                    out_.movRdxImm32(3);
                    out_.movR10Imm32(0x22);
                    out_.movR8Imm32(0xFFFFFFFFu);
                    out_.movR9Imm32(0);
                    out_.movEaxImm32(9);
                    out_.syscall();
                    out_.cmpRaxImm8(0xFF); // MAP_FAILED == -1
                    out_.jcc(J_E, noArena);
                }
                out_.leaR10Rip("__nx_arena");
                out_.movR10FromRax();
                out_.movR10Disp8FromRax(8);
                out_.call("main");
                if (target_ == NativeTarget::WindowsX64) {
                    // ExitProcess(exitCode). The caller's stack is balanced again, so
                    // rsp is 8 (mod 16); reserve 8 bytes padding + 32 bytes shadow.
                    out_.movEcxEax();
                    out_.subRsp(40);
                    out_.callIAT("__iat_ExitProcess");
                    out_.ud2();
                } else {
                    out_.movEdiEax();
                    out_.movEaxImm32(60); // sys_exit
                    out_.syscall();
                }
                out_.def(noArena);
                out_.jmp("__nx_oom");
            }
        };

    } // namespace detail

    bool supports(const Program& program, std::string& reason) {
        reason.clear();
        if (!program.structs.empty()) {
            reason = "struct '" + program.structs.front().name +
                     "' is outside the builtin native v0 subset (use --backend llvm)";
            return false;
        }
        if (!program.enums.empty()) {
            reason = "enum '" + program.enums.front().name +
                     "' is outside the builtin native v0 subset (use --backend llvm)";
            return false;
        }
        std::map<std::string, detail::FnSig> fns;
        for (const auto& f : program.functions) {
            detail::FnSig sig;
            for (const auto& p : f.params) {
                detail::Kind k = detail::Kind::Void;
                if (!detail::kindOfType(p.type, k) || k == detail::Kind::Void) {
                    reason = "parameter '" + p.name + "' of '" + f.name +
                             "' has a type outside the builtin native v0 subset (only i64 and bool "
                             "are supported)";
                    return false;
                }
                sig.params.push_back(k);
            }
            if (!detail::kindOfType(f.ret, sig.ret)) {
                reason = "result type of '" + f.name +
                         "' is outside the builtin native v0 subset (only i64, bool, and void are "
                         "supported)";
                return false;
            }
            if (f.external) {
                reason = "extern function '" + f.name +
                         "' is outside the builtin native v0 subset (use --backend llvm)";
                return false;
            }
            fns[f.name] = sig;
        }
        if (!fns.contains("main")) {
            reason = "no main function: the builtin native v0 backend needs `fn main()` as the "
                     "entry point";
            return false;
        }
        for (const auto& f : program.functions) {
            std::map<std::string, detail::Kind> env;
            for (std::size_t i = 0; i < f.params.size(); ++i)
                env[f.params[i].name] = fns[f.name].params[i];
            if (f.body)
                detail::checkBlock(f.body.get(), env, fns, false, reason);
            if (!reason.empty()) {
                reason = "in function '" + f.name + "': " + reason;
                return false;
            }
        }
        return true;
    }

    std::vector<std::uint8_t> buildExecutable(const Program& program, NativeTarget target) {
        std::string reason;
        if (!supports(program, reason))
            throw std::runtime_error(reason);
        CodeImage image;
        if (target == NativeTarget::WindowsX64) {
            image.imports = {
                "GetStdHandle", "WriteFile", "ExitProcess", "VirtualAlloc", "ReadFile"};
        }
        detail::Generator gen(image, target);
        gen.generate(program);
        if (target == NativeTarget::WindowsX64)
            return writeWindowsPE(image);
        return writeLinuxELF(image);
    }

} // namespace nexus::native
