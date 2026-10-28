#include "native/native_generator.hpp"

#include <stdexcept>

namespace nexus::native {
    namespace detail {


        void Generator::genStmt(const Stmt* s) {
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
            if (const auto* f = dynamic_cast<const ForStmt*>(s)) {
                if (!f->rangeEnd)
                    throw std::runtime_error("builtin native v0: unsupported collection iteration");
                const std::string limitName = forLimitNames_.at(f);
                const std::string cond = out_.label("for.cond");
                const std::string body = out_.label("for.body");
                const std::string increment = out_.label("for.increment");
                const std::string end = out_.label("for.end");

                // Evaluate both bounds exactly once, before entering the loop.
                genExpr(f->start.get());
                out_.movRbpFromRax(slotOff(slots_.at(f->name)));
                genExpr(f->rangeEnd.get());
                out_.movRbpFromRax(slotOff(slots_.at(limitName)));

                out_.jmp(cond);
                out_.def(cond);
                BinaryExpr test(TokenKind::Less,
                                std::make_unique<VarExpr>(f->name),
                                std::make_unique<VarExpr>(limitName));
                genExpr(&test);
                out_.testRaxRax();
                out_.jcc(J_E, end);
                out_.def(body);
                breakStack_.push_back(end);
                continueStack_.push_back(increment);
                genBlock(f->body.get());
                breakStack_.pop_back();
                continueStack_.pop_back();

                out_.def(increment);
                BinaryExpr nextValue(TokenKind::Plus,
                                     std::make_unique<VarExpr>(f->name),
                                     std::make_unique<IntExpr>(1));
                genExpr(&nextValue);
                out_.movRbpFromRax(slotOff(slots_.at(f->name)));
                out_.jmp(cond);
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

        void Generator::genExpr(const Expr* e) {
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
                // `&&`/`||` must not evaluate the right-hand side when the
                // left-hand side already decides the result.
                if (b->op == TokenKind::AndAnd || b->op == TokenKind::OrOr) {
                    const std::string end = out_.label("logic.end");
                    genExpr(b->lhs.get());
                    out_.testRaxRax();
                    out_.jcc(b->op == TokenKind::AndAnd ? J_E : J_NE, end);
                    genExpr(b->rhs.get());
                    out_.def(end);
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
                case TokenKind::Percent: {
                    // Use the same checked failure path as the LLVM backend.
                    const std::string nonzero = out_.label("division.nonzero");
                    out_.testRcxRcx();
                    out_.jcc(J_NE, nonzero);
                    out_.jmp("__nx_divzero");
                    out_.def(nonzero);
                    out_.cqo();
                    out_.idivRcx();
                    if (b->op == TokenKind::Percent)
                        out_.movRaxRdx();
                    break;
                }
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
                case TokenKind::Ampersand:
                    out_.andRaxRcx();
                    break;
                case TokenKind::Pipe:
                    out_.orRaxRcx();
                    break;
                case TokenKind::Caret:
                    out_.xorRaxRcx();
                    break;
                case TokenKind::ShiftLeft:
                    out_.shlRaxCl();
                    break;
                case TokenKind::ShiftRight:
                    out_.sarRaxCl();
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

        void Generator::genPrint(const Expr* arg) {
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
        bool Generator::exprIsString(const Expr* e) {
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
        bool Generator::exprIsBool(const Expr* e) {
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

        Kind Generator::localKind(const std::string& name) {
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

        void Generator::findLet(const Block* b, const std::string& name, Kind& found, bool& ok) {
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

        Kind Generator::initKind(const Expr* e) {
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
        void Generator::stubCall(const std::string& target) {
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

    } // namespace detail
} // namespace nexus::native
