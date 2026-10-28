#pragma once

#include "lexer/token.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace nexus {

    // Lexer helpers shared by the split implementation files. They are inline
    // so every translation unit sees the same definition.

    inline int hexDigit(char c) {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    }

    inline void appendUtf8CodePoint(std::string& out, std::uint32_t codePoint) {
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

    inline std::runtime_error escapeError(const char* message, const SourcePos& pos) {
        return std::runtime_error(std::string(message) + " at " +
                                  std::to_string(pos.line) + ":" +
                                  std::to_string(pos.column));
    }

} // namespace nexus
