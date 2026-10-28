#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace nexus {
    namespace driver {
        namespace lsp {

struct LspDiagnostic {
    std::size_t line = 0;
    std::size_t character = 0;
    std::size_t length = 1;
    std::string code;
    std::string message;
};

std::vector<LspDiagnostic> collectDiagnostics(const std::string& source);

        } // namespace lsp
    } // namespace driver
} // namespace nexus
