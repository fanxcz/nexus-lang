#include "native/native_internal.hpp"

#include "lexer/token.hpp"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace nexus::native {
    namespace detail {

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
                case TokenKind::Ampersand:
                case TokenKind::Pipe:
                case TokenKind::Caret:
                case TokenKind::ShiftLeft:
                case TokenKind::ShiftRight:
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

        bool blockBindsName(const Block* block, const std::string& name) {
            if (!block)
                return false;
            for (const auto& statement : block->statements) {
                if (const auto* let = dynamic_cast<const LetStmt*>(statement.get())) {
                    if (let->name == name)
                        return true;
                } else if (const auto* nested = dynamic_cast<const Block*>(statement.get())) {
                    if (blockBindsName(nested, name))
                        return true;
                } else if (const auto* branch = dynamic_cast<const IfStmt*>(statement.get())) {
                    if (blockBindsName(branch->thenBlock.get(), name) ||
                        blockBindsName(branch->elseBlock.get(), name))
                        return true;
                } else if (const auto* loop = dynamic_cast<const WhileStmt*>(statement.get())) {
                    if (blockBindsName(loop->body.get(), name))
                        return true;
                } else if (const auto* loop = dynamic_cast<const ForStmt*>(statement.get())) {
                    if (loop->name == name || blockBindsName(loop->body.get(), name))
                        return true;
                }
            }
            return false;
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
            if (dynamic_cast<const DerefAssignStmt*>(s)) {
                reason = "pointer assignment is outside the builtin native v0 subset; use --backend llvm";
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
            if (dynamic_cast<const DeferStmt*>(s)) {
                reason = "defer is outside the builtin native v0 subset; use --backend llvm";
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
            if (const auto* loop = dynamic_cast<const ForStmt*>(s)) {
                if (!loop->rangeEnd || !loop->start) {
                    reason = "array/string iteration is outside the builtin native v0 subset; use --backend llvm";
                    return;
                }
                if (env.contains(loop->name)) {
                    reason = "shadowed for-loop variable '" + loop->name +
                             "' is outside the builtin native v0 subset";
                    return;
                }
                if (blockBindsName(loop->body.get(), loop->name)) {
                    reason = "shadowing the for-loop variable '" + loop->name +
                             "' inside its body is outside the builtin native v0 subset";
                    return;
                }
                const Kind start = checkExpr(loop->start.get(), env, fns, reason);
                if (!reason.empty())
                    return;
                const Kind limit = checkExpr(loop->rangeEnd.get(), env, fns, reason);
                if (!reason.empty())
                    return;
                if (start != Kind::I64 || limit != Kind::I64) {
                    reason = "numeric for-loop bounds must be i64 in the builtin native v0 subset";
                    return;
                }
                auto saved = env;
                env[loop->name] = Kind::I64;
                checkBlock(loop->body.get(), env, fns, true, reason);
                env = std::move(saved);
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

    } // namespace detail
} // namespace nexus::native
