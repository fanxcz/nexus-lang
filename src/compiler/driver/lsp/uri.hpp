#pragma once

#include <filesystem>
#include <string>

namespace nexus {
    namespace driver {
        namespace lsp {

std::filesystem::path pathFromUri(const std::string& uri);
std::string uriFromPath(const std::filesystem::path& inputPath);

        } // namespace lsp
    } // namespace driver
} // namespace nexus
