#include "parser/parser.hpp"

#include "parser/parser_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace nexus {

    std::unique_ptr<Block> Parser::parseBlock() {
        RecursionGuard guard(recursionDepth_);
        const std::size_t first = i_;
        expect(TokenKind::LBrace);
        auto b = std::make_unique<Block>();

        const auto isStatementStart = [](TokenKind kind) {
            switch (kind) {
            case TokenKind::KwLet:
            case TokenKind::KwConst:
            case TokenKind::KwReturn:
            case TokenKind::KwIf:
            case TokenKind::KwWhile:
            case TokenKind::KwFor:
            case TokenKind::KwBreak:
            case TokenKind::KwContinue:
            case TokenKind::KwDefer:
            case TokenKind::KwMatch:
            case TokenKind::KwUnsafe:
                return true;
            default:
                return false;
            }
        };

        while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End) {
            const std::size_t statementStart = i_;
            try {
                b->statements.push_back(parseStmt());
            } catch (const std::exception& error) {
                // A missing delimiter at EOF is usually a broken enclosing block;
                // let the outer parser report it once rather than emitting cascades.
                if (cur().kind == TokenKind::End)
                    throw;

                recoveredErrors_.emplace_back(error.what());

                // NEXUS permits omitted semicolons, so recover at explicit statement
                // terminators or at an unmistakable statement keyword. Do not retry
                // the token that caused this error.
                while (cur().kind != TokenKind::End && cur().kind != TokenKind::RBrace &&
                       cur().kind != TokenKind::Semicolon) {
                    if (i_ > statementStart && isStatementStart(cur().kind))
                        break;
                    ++i_;
                }
                if (cur().kind == TokenKind::Semicolon) {
                    ++i_;
                } else if (i_ == statementStart && cur().kind != TokenKind::RBrace &&
                           cur().kind != TokenKind::End) {
                    ++i_;
                }
            }
        }

        expect(TokenKind::RBrace);
        b->span = spanForTokens(first, i_);
        return b;
    }
    Stmt::Ptr Parser::parseStmt() {
        const std::size_t first = i_;
        auto statement = parseStmtImpl();
        if (statement)
            statement->span = spanForTokens(first, i_);
        return statement;
    }

    Stmt::Ptr Parser::parseStmtImpl() {
        RecursionGuard guard(recursionDepth_);

        bool isConst = accept(TokenKind::KwConst);
        if (isConst || accept(TokenKind::KwLet)) {
            bool mut = !isConst && accept(TokenKind::KwMut);
            auto n = expectIdentifier();
            Type ty;
            if (accept(TokenKind::Colon))
                ty = parseType();
            expect(TokenKind::Equal);
            auto e = parseExpr();
            accept(TokenKind::Semicolon);
            auto s = std::make_unique<LetStmt>();
            s->name = n;
            s->mut = mut;
            s->type = ty;
            s->init = std::move(e);
            return s;
        }
        if (accept(TokenKind::KwReturn)) {
            auto e = cur().kind == TokenKind::RBrace ? nullptr : parseExpr();
            accept(TokenKind::Semicolon);
            auto s = std::make_unique<ReturnStmt>();
            s->expr = std::move(e);
            return s;
        }
        if (accept(TokenKind::KwDefer)) {
            auto call = parseExpr();
            accept(TokenKind::Semicolon);
            auto s = std::make_unique<DeferStmt>();
            s->call = std::move(call);
            return s;
        }
        if (accept(TokenKind::KwIf)) {

            auto c = parseExpr();

            auto t = parseBlock();

            std::unique_ptr<Block> e;

            if (accept(TokenKind::KwElse)) {

                if (accept(TokenKind::KwIf)) {

                    auto nested = std::make_unique<IfStmt>();

                    nested->cond = parseExpr();

                    nested->thenBlock = parseBlock();

                    if (accept(TokenKind::KwElse)) {

                        if (accept(TokenKind::KwIf)) {

                            --i_;

                            nested->elseBlock = std::make_unique<Block>();

                            nested->elseBlock->statements.push_back(parseStmt());

                        } else {

                            nested->elseBlock = parseBlock();
                        }
                    }
                    e = std::make_unique<Block>();

                    e->statements.push_back(std::move(nested));

                } else {

                    e = parseBlock();
                }
            }
            auto s = std::make_unique<IfStmt>();

            s->cond = std::move(c);

            s->thenBlock = std::move(t);

            s->elseBlock = std::move(e);

            return s;
        }
        if (accept(TokenKind::KwFor)) {

            auto name = expectIdentifier();

            expect(TokenKind::KwIn);

            auto start = parseExpr();

            auto loop = std::make_unique<ForStmt>();

            loop->name = std::move(name);

            if (accept(TokenKind::Range)) {
                loop->start = std::move(start);
                loop->rangeEnd = parseExpr();
            } else {
                // `for value in array` evaluates the array once and binds each
                // element in turn.
                loop->collection = std::move(start);
            }

            loop->body = parseBlock();

            return loop;
        }
        if (accept(TokenKind::KwWhile)) {
            auto c = parseExpr();
            auto b = parseBlock();
            auto s = std::make_unique<WhileStmt>();
            s->cond = std::move(c);
            s->body = std::move(b);
            return s;
        }
        if (accept(TokenKind::KwMatch)) {
            Expr::Ptr value;
            if (cur().kind == TokenKind::Identifier && i_ + 1 < tokens_.size() &&
                tokens_[i_ + 1].kind == TokenKind::LBrace)
                value = std::make_unique<VarExpr>(expectIdentifier());
            else
                value = parseExpr();
            expect(TokenKind::LBrace);
            auto s = std::make_unique<MatchStmt>();
            s->value = std::move(value);
            while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End) {
                MatchArm arm;
                const auto nextKind = [&]() {
                    return i_ + 1 < tokens_.size() ? tokens_[i_ + 1].kind : TokenKind::End;
                };
                if (cur().kind == TokenKind::Identifier && cur().text == "_") {
                    arm.wildcard = true;
                    arm.pattern = std::make_unique<VarExpr>(expectIdentifier());
                } else if (cur().kind == TokenKind::Identifier &&
                           (nextKind() == TokenKind::KwIf || nextKind() == TokenKind::FatArrow)) {
                    // A bare identifier followed by a guard or an arrow is a
                    // binding pattern, not a reference to an existing variable.
                    arm.binding = expectIdentifier();
                } else {
                    arm.pattern = parseExpr();
                    if (auto* ev = dynamic_cast<EnumVariantExpr*>(arm.pattern.get())) {
                        if (ev->payload) {
                            if (auto* ve = dynamic_cast<VarExpr*>(ev->payload.get())) {
                                arm.binding = ve->name;
                                ev->payload.reset();
                            }
                        }
                    }
                    if (accept(TokenKind::Range))
                        arm.rangeEnd = parseExpr();
                }
                if (accept(TokenKind::KwIf))
                    arm.guard = parseExpr();
                expect(TokenKind::FatArrow);
                arm.body = parseBlock();
                s->arms.push_back(std::move(arm));
                accept(TokenKind::Comma);
            }
            expect(TokenKind::RBrace);
            return s;
        }
        if (accept(TokenKind::KwBreak)) {
            accept(TokenKind::Semicolon);
            return std::make_unique<BreakStmt>();
        }
        if (accept(TokenKind::KwContinue)) {
            accept(TokenKind::Semicolon);
            return std::make_unique<ContinueStmt>();
        }
        auto lhs = parseExpr();

        TokenKind assignOp = TokenKind::Equal;
        bool hasAssign = false;

        if (accept(TokenKind::Equal)) {
            hasAssign = true;
            assignOp = TokenKind::Equal;
        } else if (accept(TokenKind::PlusEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Plus;
        } else if (accept(TokenKind::MinusEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Minus;
        } else if (accept(TokenKind::StarEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Star;
        } else if (accept(TokenKind::SlashEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Slash;
        } else if (accept(TokenKind::PercentEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Percent;
        } else if (accept(TokenKind::AmpEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Ampersand;
        } else if (accept(TokenKind::PipeEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Pipe;
        } else if (accept(TokenKind::CaretEqual)) {
            hasAssign = true;
            assignOp = TokenKind::Caret;
        } else if (accept(TokenKind::ShiftLeftEqual)) {
            hasAssign = true;
            assignOp = TokenKind::ShiftLeft;
        } else if (accept(TokenKind::ShiftRightEqual)) {
            hasAssign = true;
            assignOp = TokenKind::ShiftRight;
        }
        if (hasAssign) {
            auto rhs = parseExpr();
            accept(TokenKind::Semicolon);
            if (auto* v = dynamic_cast<VarExpr*>(lhs.get())) {
                auto s = std::make_unique<AssignStmt>();
                s->name = v->name;
                s->value = (assignOp == TokenKind::Equal)
                               ? std::move(rhs)
                               : std::make_unique<BinaryExpr>(
                                     assignOp, std::make_unique<VarExpr>(v->name), std::move(rhs));
                return s;
            }
            if (auto* m = dynamic_cast<MemberExpr*>(lhs.get())) {
                auto s = std::make_unique<MemberAssignStmt>();
                s->object = std::move(m->object);
                s->member = m->member;
                s->value = std::move(rhs);
                s->op = assignOp;
                return s;
            }
            if (auto* ix = dynamic_cast<IndexExpr*>(lhs.get())) {
                auto s = std::make_unique<IndexAssignStmt>();
                s->object = std::move(ix->object);
                s->index = std::move(ix->index);
                s->value = std::move(rhs);
                s->op = assignOp;
                return s;
            }
            if (auto* deref = dynamic_cast<UnaryExpr*>(lhs.get());
                deref && deref->op == TokenKind::Star) {
                auto s = std::make_unique<DerefAssignStmt>();
                s->target = std::move(deref->rhs);
                s->value = std::move(rhs);
                s->op = assignOp;
                return s;
            }
            throw std::runtime_error("left side of assignment is not assignable");
        }
        accept(TokenKind::Semicolon);
        auto s = std::make_unique<ExprStmt>();
        s->expr = std::move(lhs);
        return s;
    }

} // namespace nexus
