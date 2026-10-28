#include "lexer/lexer.hpp"

#include "lexer/lexer_internal.hpp"

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace nexus {

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
                } else if (peek() == '<') {
                    take();
                    if (peek() == '=') {
                        take();
                        out.push_back({TokenKind::ShiftLeftEqual, "<<=", p, pos_, true});
                    } else
                        out.push_back({TokenKind::ShiftLeft, "<<", p, pos_, true});
                } else
                    out.push_back({TokenKind::Less, "<", p, pos_, true});
                break;
            case '>':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::GreaterEqual, ">=", p, pos_, true});
                } else if (peek() == '>') {
                    take();
                    if (peek() == '=') {
                        take();
                        out.push_back({TokenKind::ShiftRightEqual, ">>=", p, pos_, true});
                    } else
                        out.push_back({TokenKind::ShiftRight, ">>", p, pos_, true});
                } else
                    out.push_back({TokenKind::Greater, ">", p, pos_, true});
                break;
            case '&':
                if (peek() == '&') {
                    take();
                    out.push_back({TokenKind::AndAnd, "&&", p, pos_, true});
                } else if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::AmpEqual, "&=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Ampersand, "&", p, pos_, true});
                break;
            case '|':
                if (peek() == '|') {
                    take();
                    out.push_back({TokenKind::OrOr, "||", p, pos_, true});
                } else if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::PipeEqual, "|=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Pipe, "|", p, pos_, true});
                break;
            case '^':
                if (peek() == '=') {
                    take();
                    out.push_back({TokenKind::CaretEqual, "^=", p, pos_, true});
                } else
                    out.push_back({TokenKind::Caret, "^", p, pos_, true});
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
            case '?':
                out.push_back({TokenKind::Question, "?", p, pos_, true});
                break;
            default:
                throw std::runtime_error(std::string("unexpected character '") + c + "' at " +
                                         std::to_string(p.line) + ":" + std::to_string(p.column));
            }
        }
        return out;
    }

} // namespace nexus
