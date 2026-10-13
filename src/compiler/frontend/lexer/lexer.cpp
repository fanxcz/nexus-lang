#include "lexer/lexer.hpp"

#include <cctype>
#include <stdexcept>

namespace nexus {

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
        return {k, s, p};
    }
    Token Lexer::number() {
        auto p = pos_;
        std::string s;
        while (std::isdigit(static_cast<unsigned char>(peek())))
            s += take();
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
            s += take();
            while (std::isdigit(static_cast<unsigned char>(peek())))
                s += take();
            return {TokenKind::Float, s, p};
        }
        return {TokenKind::Integer, s, p};
    }
    Token Lexer::string() {
        auto p = pos_;
        take();
        std::string s;
        while (peek() && peek() != '"') {
            char c = take();
            if (c == '\\') {
                char e = take();
                switch (e) {
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
                default:
                    throw std::runtime_error("unknown string escape at " +
                                             std::to_string(pos_.line) + ":" +
                                             std::to_string(pos_.column));
                }
            } else
                s += c;
        }
        if (peek() != '"')
            throw std::runtime_error("unterminated string at " + std::to_string(pos_.line) + ":" +
                                     std::to_string(pos_.column));
        take();
        return {TokenKind::String, s, p};
    }
    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> out;
        for (;;) {
            skipWhitespaceAndComments();
            auto p = pos_;
            char c = peek();
            if (!c) {
                out.push_back({TokenKind::End, "", p});
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
                    out.push_back({TokenKind::PlusEqual, "+=", p});
                } else
                    out.push_back({TokenKind::Plus, "+", p});
                break;
            case '-':
                if (peek() == '>') {
                    take();
                    out.push_back({TokenKind::Arrow, "->", p});
                } else if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::MinusEqual, "-=", p});
                } else
                    out.push_back({TokenKind::Minus, "-", p});
                break;
            case '*':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::StarEqual, "*=", p});
                } else
                    out.push_back({TokenKind::Star, "*", p});
                break;
            case '/':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::SlashEqual, "/=", p});
                } else
                    out.push_back({TokenKind::Slash, "/", p});
                break;
            case '%':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::PercentEqual, "%=", p});
                } else
                    out.push_back({TokenKind::Percent, "%", p});
                break;
            case '=':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::EqualEqual, "==", p});
                } else if (peek() == '>') {
                    take();
                    out.push_back({TokenKind::FatArrow, "=>", p});
                } else
                    out.push_back({TokenKind::Equal, "=", p});
                break;
            case '!':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::BangEqual, "!=", p});
                } else
                    out.push_back({TokenKind::Bang, "!", p});
                break;
            case '<':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::LessEqual, "<=", p});
                } else
                    out.push_back({TokenKind::Less, "<", p});
                break;
            case '>':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::GreaterEqual, ">=", p});
                } else
                    out.push_back({TokenKind::Greater, ">", p});
                break;
            case '&':
                if (peek() == '&') {
                    take();
                    out.push_back({TokenKind::AndAnd, "&&", p});
                } else
                    out.push_back({TokenKind::Ampersand, "&", p});
                break;
            case '|':
                if (peek() == '|') {
                    take();
                    out.push_back({TokenKind::OrOr, "||", p});
                } else
                    throw std::runtime_error("expected '|' or '||' at " + std::to_string(p.line) +
                                             ":" + std::to_string(p.column));
                break;
            case '(':
                out.push_back({TokenKind::LParen, "(", p});
                break;
            case ')':
                out.push_back({TokenKind::RParen, ")", p});
                break;
            case '{':
                out.push_back({TokenKind::LBrace, "{", p});
                break;
            case '}':
                out.push_back({TokenKind::RBrace, "}", p});
                break;
            case '[':
                out.push_back({TokenKind::LBracket, "[", p});
                break;
            case ']':
                out.push_back({TokenKind::RBracket, "]", p});
                break;
            case ',':
                out.push_back({TokenKind::Comma, ",", p});
                break;
            case ':':
                if (peek() == ':') {
                    take();
                    out.push_back({TokenKind::ColonColon, "::", p});
                } else
                    out.push_back({TokenKind::Colon, ":", p});
                break;
            case '.':
                if (peek() == '.') {
                    take();
                    out.push_back({TokenKind::Range, "..", p});
                } else
                    out.push_back({TokenKind::Dot, ".", p});
                break;
            case ';':
                out.push_back({TokenKind::Semicolon, ";", p});
                break;
            default:
                throw std::runtime_error(std::string("unexpected character '") + c + "' at " +
                                         std::to_string(p.line) + ":" + std::to_string(p.column));
            }
        }
        return out;
    }
} // namespace nexus
