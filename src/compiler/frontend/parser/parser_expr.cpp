#include "parser/parser.hpp"

#include "parser/parser_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace nexus {

    int Parser::precedence(TokenKind k) const {
        switch (k) {
        case TokenKind::OrOr:
            return 1;
        case TokenKind::AndAnd:
            return 2;
        case TokenKind::Pipe:
            return 3;
        case TokenKind::Caret:
            return 4;
        case TokenKind::Ampersand:
            return 5;
        case TokenKind::EqualEqual:
        case TokenKind::BangEqual:
            return 6;
        case TokenKind::Less:
        case TokenKind::LessEqual:
        case TokenKind::Greater:
        case TokenKind::GreaterEqual:
            return 7;
        case TokenKind::ShiftLeft:
        case TokenKind::ShiftRight:
            return 8;
        case TokenKind::Plus:
        case TokenKind::Minus:
            return 9;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
            return 10;
        default:
            return -1;
        }
    }
    Expr::Ptr Parser::parsePrimary() {
        const std::size_t first = i_;
        auto expression = parsePrimaryImpl();
        if (expression)
            expression->span = spanForTokens(first, i_);
        return expression;
    }

    Expr::Ptr Parser::parsePrimaryImpl() {
        if (cur().kind == TokenKind::Integer) {
            const Token literal = expect(TokenKind::Integer);
            try {
                return std::make_unique<IntExpr>(std::stoll(removeNumericSeparators(literal.text)));
            } catch (const std::exception&) {
                throw std::runtime_error("integer literal out of range at " +
                                         std::to_string(literal.pos.line) + ":" +
                                         std::to_string(literal.pos.column));
            }
        }
        if (cur().kind == TokenKind::Float) {
            const Token literal = expect(TokenKind::Float);
            try {
                return std::make_unique<FloatExpr>(std::stod(removeNumericSeparators(literal.text)));
            } catch (const std::exception&) {
                throw std::runtime_error("floating-point literal out of range at " +
                                         std::to_string(literal.pos.line) + ":" +
                                         std::to_string(literal.pos.column));
            }
        }
        if (cur().kind == TokenKind::String) {
            return std::make_unique<StringExpr>(expect(TokenKind::String).text);
        }
        if (accept(TokenKind::KwTrue))
            return std::make_unique<BoolExpr>(true);
        if (accept(TokenKind::KwFalse))
            return std::make_unique<BoolExpr>(false);

        if (cur().kind == TokenKind::Identifier || cur().kind == TokenKind::KwPrint) {

            auto n = expect(cur().kind).text;

            if (cur().kind == TokenKind::ColonColon) {
                ++i_;
                auto v = expectIdentifier();
                auto e = std::make_unique<EnumVariantExpr>(n, v);
                if (cur().kind == TokenKind::LParen) {
                    ++i_;
                    GroupGuard group(groupingDepth_);
                    e->payload = parseExpr();
                    expect(TokenKind::RParen);
                }
                return e;
            }
            if (cur().kind == TokenKind::LBrace && i_ + 2 < tokens_.size() &&
                tokens_[i_ + 1].kind == TokenKind::Identifier &&
                tokens_[i_ + 2].kind == TokenKind::Colon) {
                auto s = std::make_unique<StructInitExpr>(n);
                expect(TokenKind::LBrace);
                GroupGuard group(groupingDepth_);
                while (cur().kind != TokenKind::RBrace) {
                    auto f = expectIdentifier();
                    expect(TokenKind::Colon);
                    s->fields.push_back({f, parseExpr()});
                    if (!accept(TokenKind::Comma))
                        break;
                }
                expect(TokenKind::RBrace);
                return s;
            }
            return std::make_unique<VarExpr>(n);
        }
        if (accept(TokenKind::LBracket)) {
            GroupGuard group(groupingDepth_);
            auto a = std::make_unique<ArrayExpr>();
            if (cur().kind != TokenKind::RBracket) {
                for (;;) {
                    a->elements.push_back(parseExpr());
                    if (!accept(TokenKind::Comma))
                        break;
                }
            }
            expect(TokenKind::RBracket);
            return a;
        }
        if (accept(TokenKind::LParen)) {
            GroupGuard group(groupingDepth_);
            auto e = parseExpr();
            expect(TokenKind::RParen);
            return e;
        }
        throw std::runtime_error(
            std::string("expected expression at ") + std::to_string(cur().pos.line) + ":" +
            std::to_string(cur().pos.column) + " (got " + tokenName(cur().kind) + ")");
    }
    // True when a token can legally be the last token of an expression, so a
    // following line may start a new statement.
    static bool endsExpression(TokenKind kind) {
        switch (kind) {
        case TokenKind::Identifier:
        case TokenKind::Integer:
        case TokenKind::Float:
        case TokenKind::String:
        case TokenKind::KwTrue:
        case TokenKind::KwFalse:
        case TokenKind::RParen:
        case TokenKind::RBracket:
        case TokenKind::Question:
            return true;
        default:
            return false;
        }
    }

    static std::string qualifiedName(const Expr* e) {
        if (auto* v = dynamic_cast<const VarExpr*>(e))
            return v->name;
        if (auto* m = dynamic_cast<const MemberExpr*>(e)) {
            std::string base = qualifiedName(m->object.get());
            return base + "." + m->member;
        }
        return "";
    }

    Expr::Ptr Parser::parsePostfix(Expr::Ptr e) {
        const SourcePos start =
            e && e->span.valid ? e->span.start : (i_ > 0 ? tokens_[i_ - 1].pos : cur().pos);
        auto expression = parsePostfixImpl(std::move(e));
        if (expression) {
            expression->span.start = start;
            expression->span.end = i_ > 0 ? positionAfter(tokens_[i_ - 1]) : start;
            expression->span.valid = true;
        }
        return expression;
    }

    Expr::Ptr Parser::parsePostfixImpl(Expr::Ptr e) {
        for (;;) {
            if (accept(TokenKind::LParen)) {
                std::string callee = qualifiedName(e.get());
                if (callee.empty())
                    throw std::runtime_error(
                        "only named functions or namespaced calls can be called");
                GroupGuard group(groupingDepth_);
                std::vector<Expr::Ptr> a;
                if (cur().kind != TokenKind::RParen) {
                    for (;;) {
                        a.push_back(parseExpr());
                        if (!accept(TokenKind::Comma))
                            break;
                    }
                }
                expect(TokenKind::RParen);
                e = std::make_unique<CallExpr>(callee, std::move(a));
                continue;
            }
            if (accept(TokenKind::Dot)) {
                auto m = expectIdentifier();
                e = std::make_unique<MemberExpr>(std::move(e), m);
                continue;
            }
            if (accept(TokenKind::LBracket)) {
                GroupGuard group(groupingDepth_);
                auto idx = parseExpr();
                expect(TokenKind::RBracket);
                e = std::make_unique<IndexExpr>(std::move(e), std::move(idx));
                continue;
            }
            if (accept(TokenKind::Question)) {
                e = std::make_unique<TryExpr>(std::move(e));
                continue;
            }
            break;
        }
        return e;
    }
    Expr::Ptr Parser::parseUnary() {
        const std::size_t first = i_;
        auto expression = parseUnaryImpl();
        if (expression)
            expression->span = spanForTokens(first, i_);
        return expression;
    }

    Expr::Ptr Parser::parseUnaryImpl() {
        RecursionGuard guard(recursionDepth_);
        if (accept(TokenKind::Minus))
            return std::make_unique<UnaryExpr>(TokenKind::Minus, parseUnary());
        if (accept(TokenKind::Bang))
            return std::make_unique<UnaryExpr>(TokenKind::Bang, parseUnary());
        if (accept(TokenKind::Ampersand))
            return std::make_unique<UnaryExpr>(TokenKind::Ampersand, parseUnary());
        if (accept(TokenKind::Star))
            return std::make_unique<UnaryExpr>(TokenKind::Star, parseUnary());
        return parsePostfix(parsePrimary());
    }
    Expr::Ptr Parser::parseExpr(int minPrec) {
        const std::size_t first = i_;
        auto expression = parseExprImpl(minPrec);
        if (expression)
            expression->span = spanForTokens(first, i_);
        return expression;
    }

    Expr::Ptr Parser::parseExprImpl(int minPrec) {
        RecursionGuard guard(recursionDepth_);
        auto lhs = parseUnary();
        while (true) {
            int p = precedence(cur().kind);
            if (p < minPrec)
                break;
            // NEXUS allows optional semicolons, so a binary operator that
            // starts a new line must terminate the expression when the
            // previous line can already stand on its own. Without this rule
            // `a = b` followed by `-1` silently parses as `a = b - 1`, and
            // `*p = 10` directly after a `let` line is misparsed entirely.
            const auto& previous = tokens_[i_ - 1];
            const std::size_t previousLine =
                previous.hasEndPos ? previous.endPos.line : previous.pos.line;
            if (groupingDepth_ == 0 && cur().pos.line > previousLine &&
                endsExpression(previous.kind))
                break;
            auto op = cur().kind;
            ++i_;
            auto rhs = parseExpr(p + 1);
            lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
        }
        return lhs;
    }

} // namespace nexus
