#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace nexus {
    namespace driver {
        namespace lsp {

std::size_t utf16Length(const std::string& text);
std::size_t byteOffsetForUtf16(const std::string& text, std::size_t wanted);
bool identifierCharacter(unsigned char ch);
std::vector<std::string> splitLines(const std::string& text);
std::string joinRange(const std::vector<std::string>& lines, std::size_t first,
                      std::size_t last);
std::size_t editDistance(const std::string& left, const std::string& right);

        } // namespace lsp
    } // namespace driver
} // namespace nexus
