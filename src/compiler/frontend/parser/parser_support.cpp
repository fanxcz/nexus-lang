#include "parser/parser.hpp"

#include "parser/parser_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <sstream>
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

    SourcePos Parser::positionAfter(const Token& token) const {
        if (token.hasEndPos)
            return token.endPos;

        SourcePos end = token.pos;
        for (const unsigned char ch : token.text) {
            ++end.offset;
            if (ch == '\n') {
                ++end.line;
                end.column = 1;
            } else {
                ++end.column;
            }
        }
        return end;
    }

    SourceSpan Parser::spanForTokens(std::size_t first, std::size_t afterLast) const {
        SourceSpan span;
        if (first >= tokens_.size())
            return span;
        const std::size_t last =
            afterLast > first ? std::min(afterLast - 1, tokens_.size() - 1) : first;
        span.start = tokens_[first].pos;
        span.end = positionAfter(tokens_[last]);
        span.valid = true;
        span.sourcePath = sourcePath_;
        return span;
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
                const auto parsed = std::stoull(removeNumericSeparators(lengthToken.text));
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

} // namespace nexus
