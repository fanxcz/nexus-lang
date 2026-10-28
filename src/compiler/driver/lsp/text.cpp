#include "driver/lsp/text.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace nexus {
    namespace driver {
        namespace lsp {

        std::size_t utf16Length(const std::string& text) {
            std::size_t units = 0;
            for (std::size_t i = 0; i < text.size();) {
                const unsigned char ch = static_cast<unsigned char>(text[i]);
                std::size_t width = 1;
                if ((ch & 0xe0) == 0xc0)
                    width = 2;
                else if ((ch & 0xf0) == 0xe0)
                    width = 3;
                else if ((ch & 0xf8) == 0xf0)
                    width = 4;
                if (i + width > text.size())
                    width = 1;
                units += width == 4 ? 2 : 1;
                i += width;
            }
            return units;
        }

        std::size_t byteOffsetForUtf16(const std::string& text, std::size_t wanted) {
            std::size_t units = 0;
            for (std::size_t i = 0; i < text.size();) {
                const unsigned char ch = static_cast<unsigned char>(text[i]);
                std::size_t width = 1;
                if ((ch & 0xe0) == 0xc0)
                    width = 2;
                else if ((ch & 0xf0) == 0xe0)
                    width = 3;
                else if ((ch & 0xf8) == 0xf0)
                    width = 4;
                if (i + width > text.size())
                    width = 1;
                const std::size_t step = width == 4 ? 2 : 1;
                if (units + step > wanted)
                    return i;
                if (units + step == wanted)
                    return i + width;
                units += step;
                i += width;
            }
            return text.size();
        }

        bool identifierCharacter(unsigned char ch) {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                   (ch >= '0' && ch <= '9') || ch == '_';
        }

        std::vector<std::string> splitLines(const std::string& text) {
                            std::vector<std::string> lines;
                            std::size_t start = 0;
                            for (;;) {
                                const std::size_t newline = text.find('\n', start);
                                if (newline == std::string::npos) {
                                    lines.push_back(text.substr(start));
                                    break;
                                }
                                lines.push_back(text.substr(start, newline - start));
                                start = newline + 1;
                                if (start == text.size()) {
                                    lines.emplace_back();
                                    break;
                                }
                            }
                            for (auto& line : lines) {
                                if (!line.empty() && line.back() == '\r')
                                    line.pop_back();
                            }
                            return lines;
                        }

        std::string joinRange(const std::vector<std::string>& lines, std::size_t first, std::size_t last) {
                                std::string text;
                                for (std::size_t line = first; line <= last; ++line) {
                                    if (line != first)
                                        text.push_back('\n');
                                    text += lines[line];
                                }
                                return text;
                            }

        std::size_t editDistance(const std::string& left, const std::string& right) {
                            std::vector<std::size_t> previous(right.size() + 1);
                            for (std::size_t j = 0; j <= right.size(); ++j)
                                previous[j] = j;
                            for (std::size_t i = 1; i <= left.size(); ++i) {
                                std::vector<std::size_t> current(right.size() + 1);
                                current[0] = i;
                                for (std::size_t j = 1; j <= right.size(); ++j) {
                                    const std::size_t substitution =
                                        previous[j - 1] + (left[i - 1] == right[j - 1] ? 0 : 1);
                                    current[j] = std::min(
                                        {previous[j] + 1, current[j - 1] + 1, substitution});
                                }
                                previous.swap(current);
                            }
                            return previous.back();
                        }

        } // namespace lsp
    } // namespace driver
} // namespace nexus
