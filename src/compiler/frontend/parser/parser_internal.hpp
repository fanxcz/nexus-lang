#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace nexus {

    // Parser helpers shared by the split implementation files.

    inline std::string removeNumericSeparators(const std::string& literal) {
        std::string normalized;
        normalized.reserve(literal.size());
        for (const char c : literal) {
            if (c != '_')
                normalized.push_back(c);
        }
        return normalized;
    }

    inline std::uint64_t parseUnsignedIntegerLiteral(const std::string& literal) {
        const std::string normalized = removeNumericSeparators(literal);
        int base = 10;
        std::size_t prefixLength = 0;
        if (normalized.size() >= 2 && normalized[0] == '0') {
            switch (normalized[1]) {
            case 'x':
            case 'X':
                base = 16;
                prefixLength = 2;
                break;
            case 'b':
            case 'B':
                base = 2;
                prefixLength = 2;
                break;
            case 'o':
            case 'O':
                base = 8;
                prefixLength = 2;
                break;
            default:
                break;
            }
        }

        const std::string digits = normalized.substr(prefixLength);
        if (digits.empty())
            throw std::invalid_argument("integer literal has no digits");
        std::size_t consumed = 0;
        const auto value = std::stoull(digits, &consumed, base);
        if (consumed != digits.size())
            throw std::invalid_argument("invalid integer literal");
        return value;
    }

    // Tracks explicit grouping so the newline-terminates-expression rule
    // only applies at statement level, not inside parentheses or brackets.
    struct GroupGuard {
        int& depth;
        explicit GroupGuard(int& d) : depth(d) {
            ++depth;
        }
        ~GroupGuard() {
            --depth;
        }
        GroupGuard(const GroupGuard&) = delete;
        GroupGuard& operator=(const GroupGuard&) = delete;
    };

} // namespace nexus
