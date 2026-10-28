#include "lexer/lexer.hpp"

#include "lexer/lexer_internal.hpp"

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace nexus {

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

} // namespace nexus
