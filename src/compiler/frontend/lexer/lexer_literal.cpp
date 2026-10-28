#include "lexer/lexer.hpp"

#include "lexer/lexer_internal.hpp"

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace nexus {

    Token Lexer::identOrKeyword() {
        auto p = pos_;
        std::string s;
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
            s += take();
        TokenKind k = TokenKind::Identifier;
        if (s == "fn")
            k = TokenKind::KwFn;
        else if (s == "let")
            k = TokenKind::KwLet;
        else if (s == "const")
            k = TokenKind::KwConst;
        else if (s == "mut")
            k = TokenKind::KwMut;
        else if (s == "return")
            k = TokenKind::KwReturn;
        else if (s == "if")
            k = TokenKind::KwIf;
        else if (s == "else")
            k = TokenKind::KwElse;
        else if (s == "while")
            k = TokenKind::KwWhile;
        else if (s == "for")
            k = TokenKind::KwFor;
        else if (s == "in")
            k = TokenKind::KwIn;
        else if (s == "break")
            k = TokenKind::KwBreak;
        else if (s == "continue")
            k = TokenKind::KwContinue;
        else if (s == "defer")
            k = TokenKind::KwDefer;
        else if (s == "struct")
            k = TokenKind::KwStruct;
        else if (s == "enum")
            k = TokenKind::KwEnum;
        else if (s == "import")
            k = TokenKind::KwImport;
        else if (s == "extern")
            k = TokenKind::KwExtern;
        else if (s == "unsafe")
            k = TokenKind::KwUnsafe;
        else if (s == "as")
            k = TokenKind::KwAs;
        else if (s == "match")
            k = TokenKind::KwMatch;
        else if (s == "true")
            k = TokenKind::KwTrue;
        else if (s == "false")
            k = TokenKind::KwFalse;
        else if (s == "print")
            k = TokenKind::KwPrint;
        return {k, s, p, pos_, true};
    }
    Token Lexer::number() {
        const auto p = pos_;
        std::string s;

        // Separators improve readability in large constants, but must occur
        // strictly between digits (for example, 1'000 is not this language's
        // syntax; use 1_000 instead).
        const auto consumeDigits = [this, &s]() {
            bool consumedDigit = false;
            bool previousWasDigit = false;
            while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_') {
                if (peek() == '_') {
                    if (!previousWasDigit ||
                        !std::isdigit(static_cast<unsigned char>(peek(1)))) {
                        throw std::runtime_error("misplaced '_' in numeric literal at " +
                                                 std::to_string(pos_.line) + ":" +
                                                 std::to_string(pos_.column));
                    }
                    s += take();
                    previousWasDigit = false;
                } else {
                    s += take();
                    consumedDigit = true;
                    previousWasDigit = true;
                }
            }
            return consumedDigit;
        };

        consumeDigits(); // The caller enters here only when the first character is a digit.
        bool isFloat = false;
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
            isFloat = true;
            s += take();
            consumeDigits();
        }

        // Scientific notation is a floating-point literal even without a
        // decimal point: 1e3, 2E+4, and 1.25e-3 are all valid.
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            s += take();
            if (peek() == '+' || peek() == '-')
                s += take();

            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                throw std::runtime_error("expected exponent digits at " +
                                         std::to_string(pos_.line) + ":" +
                                         std::to_string(pos_.column));
            }
            consumeDigits();
        }

        return {isFloat ? TokenKind::Float : TokenKind::Integer, s, p, pos_, true};
    }
    Token Lexer::string() {
        const auto p = pos_;
        take();
        std::string s;
        while (peek() && peek() != '"') {
            const auto charStart = pos_;
            const char c = take();
            if (c != '\\') {
                s += c;
                continue;
            }

            const char escape = take();
            switch (escape) {
            case 'n':
                s += '\n';
                break;
            case 't':
                s += '\t';
                break;
            case 'r':
                s += '\r';
                break;
            case '\\':
                s += '\\';
                break;
            case '"':
                s += '"';
                break;
            case 'u': {
                if (peek() != '{')
                    throw escapeError("Unicode escape must use \\u{...}", charStart);
                take();

                std::uint32_t codePoint = 0;
                std::size_t digits = 0;
                while (peek() && peek() != '}') {
                    const int digit = hexDigit(peek());
                    if (digit < 0 || digits >= 6)
                        throw escapeError("invalid Unicode escape", charStart);
                    codePoint = codePoint * 16u + static_cast<std::uint32_t>(digit);
                    ++digits;
                    take();
                }
                if (peek() != '}' || digits == 0)
                    throw escapeError("invalid Unicode escape", charStart);
                take();

                if (codePoint == 0)
                    throw escapeError("U+0000 is not supported in string literals", charStart);
                if (codePoint > 0x10ffff || (codePoint >= 0xd800 && codePoint <= 0xdfff))
                    throw escapeError("invalid Unicode scalar value", charStart);
                appendUtf8CodePoint(s, codePoint);
                break;
            }
            default:
                throw escapeError("unknown string escape", charStart);
            }
        }
        if (peek() != '"')
            throw std::runtime_error("unterminated string at " + std::to_string(pos_.line) + ":" +
                                     std::to_string(pos_.column));
        take();
        return {TokenKind::String, s, p, pos_, true};
    }

} // namespace nexus
