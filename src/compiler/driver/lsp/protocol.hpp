#pragma once

#include <cstddef>
#include <string>

namespace nexus {
    namespace driver {
        namespace lsp {

std::string extractString(const std::string& json, const std::string& field);
std::string extractId(const std::string& json);
std::size_t extractNumber(const std::string& json, const std::string& field);
bool supportsWatchedFilesRegistration(const std::string& json);
std::string jsonEscape(const std::string& value);
void send(const std::string& payload);

        } // namespace lsp
    } // namespace driver
} // namespace nexus
