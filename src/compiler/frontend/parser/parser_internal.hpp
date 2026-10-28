#pragma once

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
