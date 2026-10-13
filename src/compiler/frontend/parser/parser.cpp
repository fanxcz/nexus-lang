#include "parser/parser.hpp"

#include <cstdlib>
#include <limits>
#include <stdexcept>
namespace nexus {

    const Token& Parser::cur() const {
        return tokens_.at(i_);
    }
    bool Parser::accept(TokenKind k) {
        if (cur().kind == k) {
            ++i_;
            return true;
        }
        return false;
    }
    Token Parser::expect(TokenKind k) {
        if (cur().kind != k)
            throw std::runtime_error(
                std::string("expected ") + tokenName(k) + ", got " + tokenName(cur().kind) +
                " at " + std::to_string(cur().pos.line) + ":" + std::to_string(cur().pos.column));
        return tokens_[i_++];
    }
    std::string Parser::expectIdentifier() {
        return expect(TokenKind::Identifier).text;
    }
    Type Parser::parseType() {
        RecursionGuard guard(recursionDepth_);

        if (accept(TokenKind::Star))
            return Type::ptr(parseType());

        if (accept(TokenKind::LBracket)) {
            auto elem = parseType();
            expect(TokenKind::Semicolon);
            const Token lengthToken = expect(TokenKind::Integer);
            std::size_t length = 0;
            try {
                const auto parsed = std::stoull(lengthToken.text);
                if (parsed > std::numeric_limits<std::size_t>::max()) {
                    throw std::out_of_range("array length");
                }
                length = static_cast<std::size_t>(parsed);
            } catch (const std::exception&) {
                throw std::runtime_error("array length out of range at " +
                                         std::to_string(lengthToken.pos.line) + ":" +
                                         std::to_string(lengthToken.pos.column));
            }
            expect(TokenKind::RBracket);
            return Type::array(std::move(elem), length);
        }
        auto t = expectIdentifier();

        if (t == "i64" || t == "int" || t == "isize" || t == "usize")
            return Type::i64();
        if (t == "f64" || t == "float")
            return Type::f64();
        if (t == "bool")
            return Type::boolean();
        if (t == "string")
            return Type::string();
        if (t == "void")
            return Type::void_();
        return Type::named(t);
    }
    Program Parser::parseProgram() {
        Program p;
        while (cur().kind != TokenKind::End) {

            if (accept(TokenKind::KwImport)) {
                if (cur().kind == TokenKind::String) {
                    p.imports.push_back(expect(TokenKind::String).text);
                } else {
                    std::string s = expectIdentifier();
                    while (accept(TokenKind::Dot)) {
                        s += "." + expectIdentifier();
                    }
                    p.imports.push_back(s);
                }
                accept(TokenKind::Semicolon);
                continue;
            }
            if (accept(TokenKind::KwStruct)) {
                --i_;
                p.structs.push_back(parseStruct());
                continue;
            }
            if (accept(TokenKind::KwEnum)) {
                --i_;
                p.enums.push_back(parseEnum());
                continue;
            }
            if (accept(TokenKind::KwExtern)) {
                expect(TokenKind::String);
                p.functions.push_back(parseFunction(true));
                continue;
            }
            p.functions.push_back(parseFunction(false));
        }
        return p;
    }
    StructDecl Parser::parseStruct() {
        expect(TokenKind::KwStruct);
        StructDecl s;
        s.name = expectIdentifier();
        expect(TokenKind::LBrace);
        while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End) {
            auto n = expectIdentifier();
            expect(TokenKind::Colon);
            auto t = parseType();
            s.fields.push_back({n, t});
            accept(TokenKind::Comma);
            accept(TokenKind::Semicolon);
        }
        expect(TokenKind::RBrace);
        accept(TokenKind::Semicolon);
        return s;
    }
    EnumDecl Parser::parseEnum() {
        expect(TokenKind::KwEnum);
        EnumDecl e;
        e.name = expectIdentifier();
        expect(TokenKind::LBrace);
        while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End) {
            e.variants.push_back(expectIdentifier());
            accept(TokenKind::Comma);
            accept(TokenKind::Semicolon);
        }
        expect(TokenKind::RBrace);
        accept(TokenKind::Semicolon);
        return e;
    }
    Function Parser::parseFunction(bool external) {
        expect(TokenKind::KwFn);
        auto name = expectIdentifier();
        expect(TokenKind::LParen);
        std::vector<Param> ps;
        if (cur().kind != TokenKind::RParen) {
            for (;;) {
                auto n = expectIdentifier();
                expect(TokenKind::Colon);
                ps.push_back({n, parseType()});
                if (!accept(TokenKind::Comma))
                    break;
            }
        }
        expect(TokenKind::RParen);
        Type ret = Type::void_();
        if (accept(TokenKind::Arrow))
            ret = parseType();
        if (external) {
            accept(TokenKind::Semicolon);
            return {name, std::move(ps), ret, nullptr, true};
        }
        auto body = parseBlock();
        return {name, std::move(ps), ret, std::move(body), false};
    }
    std::unique_ptr<Block> Parser::parseBlock() {
        RecursionGuard guard(recursionDepth_);
        expect(TokenKind::LBrace);
        auto b = std::make_unique<Block>();
        while (cur().kind != TokenKind::RBrace && cur().kind != TokenKind::End)
            b->statements.push_back(parseStmt());
        expect(TokenKind::RBrace);
        return b;
    }
    Stmt::Ptr Parser::parseStmt() {
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

            expect(TokenKind::Range);

            auto end = parseExpr();

            auto body = parseBlock();

            auto init = std::make_unique<LetStmt>();
            init->name = name;
            init->mut = true;
            init->init = std::move(start);

            auto cond = std::make_unique<BinaryExpr>(
                TokenKind::Less, std::make_unique<VarExpr>(name), std::move(end));

            auto inc = std::make_unique<AssignStmt>();
            inc->name = name;
            inc->value = std::make_unique<BinaryExpr>(
                TokenKind::Plus, std::make_unique<VarExpr>(name), std::make_unique<IntExpr>(1));

            body->statements.push_back(std::move(inc));

            auto loop = std::make_unique<WhileStmt>();
            loop->cond = std::move(cond);
            loop->body = std::move(body);

            auto wrap = std::make_unique<Block>();
            wrap->statements.push_back(std::move(init));
            wrap->statements.push_back(std::move(loop));

            return wrap;
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
                bool wildcard = false;
                Expr::Ptr pattern;
                if (cur().kind == TokenKind::Identifier && cur().text == "_") {
                    wildcard = true;
                    pattern = std::make_unique<VarExpr>(expectIdentifier());
                } else
                    pattern = parseExpr();
                expect(TokenKind::FatArrow);
                auto body = parseBlock();
                s->arms.push_back({std::move(pattern), std::move(body), wildcard});
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
                return s;
            }
            if (auto* ix = dynamic_cast<IndexExpr*>(lhs.get())) {
                auto s = std::make_unique<IndexAssignStmt>();
                s->object = std::move(ix->object);
                s->index = std::move(ix->index);
                s->value = std::move(rhs);
                return s;
            }
            throw std::runtime_error("left side of assignment is not assignable");
        }
        accept(TokenKind::Semicolon);
        auto s = std::make_unique<ExprStmt>();
        s->expr = std::move(lhs);
        return s;
    }
    int Parser::precedence(TokenKind k) const {
        switch (k) {
        case TokenKind::OrOr:
            return 1;
        case TokenKind::AndAnd:
            return 2;
        case TokenKind::EqualEqual:
        case TokenKind::BangEqual:
            return 3;
        case TokenKind::Less:
        case TokenKind::LessEqual:
        case TokenKind::Greater:
        case TokenKind::GreaterEqual:
            return 4;
        case TokenKind::Plus:
        case TokenKind::Minus:
            return 5;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
            return 6;
        default:
            return -1;
        }
    }
    Expr::Ptr Parser::parsePrimary() {

        if (cur().kind == TokenKind::Integer) {
            const Token literal = expect(TokenKind::Integer);
            try {
                return std::make_unique<IntExpr>(std::stoll(literal.text));
            } catch (const std::exception&) {
                throw std::runtime_error("integer literal out of range at " +
                                         std::to_string(literal.pos.line) + ":" +
                                         std::to_string(literal.pos.column));
            }
        }
        if (cur().kind == TokenKind::Float) {
            const Token literal = expect(TokenKind::Float);
            try {
                return std::make_unique<FloatExpr>(std::stod(literal.text));
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
                return std::make_unique<EnumVariantExpr>(n, v);
            }
            if (cur().kind == TokenKind::LBrace && i_ + 2 < tokens_.size() &&
                tokens_[i_ + 1].kind == TokenKind::Identifier &&
                tokens_[i_ + 2].kind == TokenKind::Colon) {
                auto s = std::make_unique<StructInitExpr>(n);
                expect(TokenKind::LBrace);
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
            auto e = parseExpr();
            expect(TokenKind::RParen);
            return e;
        }
        throw std::runtime_error(
            std::string("expected expression at ") + std::to_string(cur().pos.line) + ":" +
            std::to_string(cur().pos.column) + " (got " + tokenName(cur().kind) + ")");
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
        for (;;) {
            if (accept(TokenKind::LParen)) {
                std::string callee = qualifiedName(e.get());
                if (callee.empty())
                    throw std::runtime_error(
                        "only named functions or namespaced calls can be called");
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
                auto idx = parseExpr();
                expect(TokenKind::RBracket);
                e = std::make_unique<IndexExpr>(std::move(e), std::move(idx));
                continue;
            }
            break;
        }
        return e;
    }
    Expr::Ptr Parser::parseUnary() {
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
        RecursionGuard guard(recursionDepth_);
        auto lhs = parseUnary();
        while (true) {
            int p = precedence(cur().kind);
            if (p < minPrec)
                break;
            auto op = cur().kind;
            ++i_;
            auto rhs = parseExpr(p + 1);
            lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
        }
        return lhs;
    }
} // namespace nexus
