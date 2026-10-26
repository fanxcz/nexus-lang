#include "lexer/lexer.hpp"

#include <cctype>
#include <cstdint>
#include <stdexcept>

namespace nexus {

    namespace {
        int hexDigit(char c) {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }

        void appendUtf8CodePoint(std::string& out, std::uint32_t codePoint) {
            if (codePoint <= 0x7f) {
                out.push_back(static_cast<char>(codePoint));
            } else if (codePoint <= 0x7ff) {
                out.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
                out.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            } else if (codePoint <= 0xffff) {
                out.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
                out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
                out.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            } else {
                out.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
                out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
                out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
                out.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
        }

        std::runtime_error escapeError(const char* message, const SourcePos& pos) {
            return std::runtime_error(std::string(message) + " at " +
                                      std::to_string(pos.line) + ":" +
                                      std::to_string(pos.column));
        }
    } // namespace

    const char* tokenName(TokenKind k) {

        switch (k) {

        case TokenKind::End:
            return "end";
        case TokenKind::Identifier:
            return "identifier";
        case TokenKind::Integer:
            return "integer";
        case TokenKind::Float:
            return "float";
        case TokenKind::String:
            return "string";

        case TokenKind::KwFn:
            return "fn";
        case TokenKind::KwLet:
            return "let";
        case TokenKind::KwConst:
            return "const";
        case TokenKind::KwMut:
            return "mut";
        case TokenKind::KwReturn:
            return "return";
        case TokenKind::KwIf:
            return "if";
        case TokenKind::KwElse:
            return "else";
        case TokenKind::KwWhile:
            return "while";
        case TokenKind::KwFor:
            return "for";
        case TokenKind::KwIn:
            return "in";
        case TokenKind::KwBreak:
            return "break";
        case TokenKind::KwContinue:
            return "continue";
        case TokenKind::KwDefer:
            return "defer";
        case TokenKind::KwStruct:
            return "struct";
        case TokenKind::KwEnum:
            return "enum";
        case TokenKind::KwImport:
            return "import";
        case TokenKind::KwExtern:
            return "extern";
        case TokenKind::KwUnsafe:
            return "unsafe";
        case TokenKind::KwAs:
            return "as";
        case TokenKind::KwMatch:
            return "match";
        case TokenKind::KwTrue:
            return "true";
        case TokenKind::KwFalse:
            return "false";
        case TokenKind::KwPrint:
            return "print";

        case TokenKind::Plus:
            return "+";
        case TokenKind::Minus:
            return "-";
        case TokenKind::Star:
            return "*";
        case TokenKind::Slash:
            return "/";
        case TokenKind::Percent:
            return "%";
        case TokenKind::PlusEqual:
            return "+=";
        case TokenKind::MinusEqual:
            return "-=";
        case TokenKind::StarEqual:
            return "*=";
        case TokenKind::SlashEqual:
            return "/=";
        case TokenKind::PercentEqual:
            return "%=";
        case TokenKind::Range:
            return "..";
        case TokenKind::Equal:
            return "=";
        case TokenKind::EqualEqual:
            return "==";
        case TokenKind::BangEqual:
            return "!=";
        case TokenKind::Less:
            return "<";
        case TokenKind::LessEqual:
            return "<=";
        case TokenKind::Greater:
            return ">";
        case TokenKind::GreaterEqual:
            return ">=";
        case TokenKind::Bang:
            return "!";
        case TokenKind::AndAnd:
            return "&&";
        case TokenKind::OrOr:
            return "||";
        case TokenKind::Ampersand:
            return "&";

        case TokenKind::LParen:
            return "(";
        case TokenKind::RParen:
            return ")";
        case TokenKind::LBrace:
            return "{";
        case TokenKind::RBrace:
            return "}";
        case TokenKind::LBracket:
            return "[";
        case TokenKind::RBracket:
            return "]";
        case TokenKind::Comma:
            return ",";
        case TokenKind::Colon:
            return ":";
        case TokenKind::ColonColon:
            return "::";
        case TokenKind::Dot:
            return ".";
        case TokenKind::Arrow:
            return "->";
        case TokenKind::FatArrow:
            return "=>";
        case TokenKind::Semicolon:
            return ";";
        }
        return "?";
    }
    char Lexer::peek(std::size_t n) const {
        return i_ + n < source_.size() ? source_[i_ + n] : '\0';
    }
    char Lexer::take() {
        char c = peek();
        if (c) {
            ++i_;
            if (c == '\n') {
                pos_.line++;
                pos_.column = 1;
            } else
                pos_.column++;
            pos_.offset++;
        }
        return c;
    }
    void Lexer::skipWhitespaceAndComments() {
        for (;;) {
            while (std::isspace(static_cast<unsigned char>(peek())))
                take();
            if (peek() == '/' && peek(1) == '/') {
                while (peek() && peek() != '\n')
                    take();
                continue;
            }
            if (peek() == '/' && peek(1) == '*') {
                take();
                take();
                while (peek() && !(peek() == '*' && peek(1) == '/'))
                    take();
                if (!peek())
                    throw std::runtime_error("unterminated block comment at " +
                                             std::to_string(pos_.line) + ":" +
                                             std::to_string(pos_.column));
                take();
                take();
                continue;
            }
            break;
        }
    }
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
    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> out;
        for (;;) {
            skipWhitespaceAndComments();
            auto p = pos_;
            char c = peek();
            if (!c) {
                out.push_back({TokenKind::End, "", p, pos_, true});
                break;
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                out.push_back(identOrKeyword());
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(c))) {
                out.push_back(number());
                continue;
            }
            if (c == '"') {
                out.push_back(string());
                continue;
            }
            take();
            switch (c) {
            case '+':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::PlusEqual, "+=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Plus, "+", p, pos_, true});
                break;
            case '-':
                if (peek() == '>') {
                    take();
                    out.push_back({TokenKind::Arrow, "->", p, pos_, true});
                } else if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::MinusEqual, "-=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Minus, "-", p, pos_, true});
                break;
            case '*':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::StarEqual, "*=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Star, "*", p, pos_, true});
                break;
            case '/':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::SlashEqual, "/=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Slash, "/", p, pos_, true});
                break;
            case '%':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::PercentEqual, "%=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Percent, "%", p, pos_, true});
                break;
            case '=':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::EqualEqual, "==", p, pos_, true});
                } else if (peek() == '>') {
                    take();
                    out.push_back({TokenKind::FatArrow, "=>", p, pos_, true});
                } else
                    out.push_back({TokenKind::Equal, "=", p, pos_, true});
                break;
            case '!':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::BangEqual, "!=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Bang, "!", p, pos_, true});
                break;
            case '<':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::LessEqual, "<=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Less, "<", p, pos_, true});
                break;
            case '>':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::GreaterEqual, ">=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Greater, ">", p, pos_, true});
                break;
            case '&':
                if (peek() == '&') {
                    take();
                    out.push_back({TokenKind::AndAnd, "&&", p, pos_, true});
                } else
                    out.push_back({TokenKind::Ampersand, "&", p, pos_, true});
                break;
            case '|':
                if (peek() == '|') {
                    take();
                    out.push_back({TokenKind::OrOr, "||", p, pos_, true});
                } else
                    throw std::runtime_error("expected '|' or '||' at " + std::to_string(p.line) +
                                             ":" + std::to_string(p.column));
                break;
            case '(':
                out.push_back({TokenKind::LParen, "(", p, pos_, true});
                break;
            case ')':
                out.push_back({TokenKind::RParen, ")", p, pos_, true});
                break;
            case '{':
                out.push_back({TokenKind::LBrace, "{", p, pos_, true});
                break;
            case '}':
                out.push_back({TokenKind::RBrace, "}", p, pos_, true});
                break;
            case '[':
                out.push_back({TokenKind::LBracket, "[", p, pos_, true});
                break;
            case ']':
                out.push_back({TokenKind::RBracket, "]", p, pos_, true});
                break;
            case ',':
                out.push_back({TokenKind::Comma, ",", p, pos_, true});
                break;
            case ':':
                if (peek() == ':') {
                    take();
                    out.push_back({TokenKind::ColonColon, "::", p, pos_, true});
                } else
                    out.push_back({TokenKind::Colon, ":", p, pos_, true});
                break;
            case '.':
                if (peek() == '.') {
                    take();
                    out.push_back({TokenKind::Range, "..", p, pos_, true});
                } else
                    out.push_back({TokenKind::Dot, ".", p, pos_, true});
                break;
            case ';':
                out.push_back({TokenKind::Semicolon, ";", p, pos_, true});
                break;
            default:
                throw std::runtime_error(std::string("unexpected character '") + c + "' at " +
                                         std::to_string(p.line) + ":" + std::to_string(p.column));
            }
        }
        return out;
    }
} // namespace nexus
