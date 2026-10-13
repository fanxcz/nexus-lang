#pragma once

#include <string>

namespace nexus {

// Format NEXUS source using the conservative indentation and whitespace rules
// shared by the CLI and Language Server Protocol implementation.
std::string formatNexusSource(const std::string& source);

} // namespace nexus
