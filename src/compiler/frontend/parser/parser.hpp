#pragma once
#include "ast/ast.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>
namespace nexus {

    class Parser {

    public:
        explicit Parser(std::vector<Token> t) : tokens_(std::move(t)) {}
        Program parseProgram();

    private:
        // Bound recursive descent on untrusted source so malformed nesting
        // fails with a diagnostic rather than exhausting the process stack.
        class RecursionGuard {
            std::size_t& depth_;
            static constexpr std::size_t maxDepth_ = 256;

        public:
            explicit RecursionGuard(std::size_t& depth) : depth_(depth) {
                if (depth_ >= maxDepth_) {
                    throw std::runtime_error("maximum parser nesting depth (256) exceeded");
                }
                ++depth_;
            }
            ~RecursionGuard() {
                --depth_;
            }
            RecursionGuard(const RecursionGuard&) = delete;
            RecursionGuard& operator=(const RecursionGuard&) = delete;
        };

        std::vector<Token> tokens_;
        std::vector<std::string> recoveredErrors_;
        size_t i_ = 0;
        std::size_t recursionDepth_ = 0;
        // Depth of explicit (), [], and {} grouping. A binary operator that
        // starts a new line only terminates an expression at depth zero.
        int groupingDepth_ = 0;

        const Token& cur() const;
        bool accept(TokenKind);
        Token expect(TokenKind);

        std::string expectIdentifier();

        Function parseFunction(bool external = false);
        StructDecl parseStruct();
        EnumDecl parseEnum();

        std::unique_ptr<Block> parseBlock();
        Stmt::Ptr parseStmt();
        Stmt::Ptr parseStmtImpl();

        Expr::Ptr parseExpr(int minPrec = 0);
        Expr::Ptr parseExprImpl(int minPrec = 0);
        Expr::Ptr parseUnary();
        Expr::Ptr parseUnaryImpl();
        Expr::Ptr parsePostfix(Expr::Ptr);
        Expr::Ptr parsePrimary();
        Expr::Ptr parsePrimaryImpl();
        Expr::Ptr parsePostfixImpl(Expr::Ptr);

        SourcePos positionAfter(const Token&) const;
        SourceSpan spanForTokens(std::size_t first, std::size_t afterLast) const;

        int precedence(TokenKind) const;
        Type parseType();
    };

} // namespace nexus
