#include "driver/lsp/protocol.hpp"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>

namespace nexus {
    namespace driver {
        namespace lsp {

        namespace {
            int hexDigit(char value) {
                if (value >= '0' && value <= '9')
                    return value - '0';
                if (value >= 'a' && value <= 'f')
                    return value - 'a' + 10;
                if (value >= 'A' && value <= 'F')
                    return value - 'A' + 10;
                return -1;
            }

            bool readHexQuad(const std::string& json, std::size_t& position,
                             std::uint32_t& value) {
                if (position + 4 > json.size())
                    return false;
                value = 0;
                for (int i = 0; i < 4; ++i) {
                    const int digit = hexDigit(json[position++]);
                    if (digit < 0)
                        return false;
                    value = (value << 4) | static_cast<std::uint32_t>(digit);
                }
                return true;
            }

            void appendUtf8(std::string& output, std::uint32_t codepoint) {
                if (codepoint <= 0x7f) {
                    output.push_back(static_cast<char>(codepoint));
                } else if (codepoint <= 0x7ff) {
                    output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
                    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
                } else if (codepoint <= 0xffff) {
                    output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
                    output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
                } else {
                    output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
                    output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
                    output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
                }
            }
        } // namespace

        std::string extractString(const std::string& json, const std::string& field) {
            const std::string key = "\"" + field + "\"";
            auto pos = json.find(key);
            if (pos == std::string::npos)
                return std::string{};
            pos = json.find(':', pos + key.size());
            if (pos == std::string::npos)
                return std::string{};
            ++pos;
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
                ++pos;
            if (pos >= json.size() || json[pos] != '"')
                return std::string{};

            std::string value;
            for (++pos; pos < json.size(); ++pos) {
                const char current = json[pos];
                if (current == '"')
                    return value;
                if (static_cast<unsigned char>(current) < 0x20)
                    return std::string{};
                if (current != '\\') {
                    value.push_back(current);
                    continue;
                }

                if (++pos >= json.size())
                    return std::string{};
                switch (json[pos]) {
                case '"':
                    value.push_back('"');
                    break;
                case '\\':
                    value.push_back('\\');
                    break;
                case '/':
                    value.push_back('/');
                    break;
                case 'b':
                    value.push_back('\b');
                    break;
                case 'f':
                    value.push_back('\f');
                    break;
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                case 'u': {
                    ++pos;
                    std::uint32_t codepoint = 0;
                    if (!readHexQuad(json, pos, codepoint))
                        return std::string{};

                    if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
                        if (pos + 2 > json.size() || json[pos] != '\\' || json[pos + 1] != 'u')
                            return std::string{};
                        pos += 2;
                        std::uint32_t lowSurrogate = 0;
                        if (!readHexQuad(json, pos, lowSurrogate) ||
                            lowSurrogate < 0xdc00 || lowSurrogate > 0xdfff)
                            return std::string{};
                        codepoint = 0x10000 +
                                    ((codepoint - 0xd800) << 10) +
                                    (lowSurrogate - 0xdc00);
                    } else if (codepoint >= 0xdc00 && codepoint <= 0xdfff) {
                        return std::string{};
                    }
                    appendUtf8(value, codepoint);
                    pos -= 1;
                    break;
                }
                default:
                    return std::string{};
                }
            }
            return std::string{};
        }

        std::string extractId(const std::string& json) {
            const auto key = json.find("\"id\"");
            if (key == std::string::npos)
                return std::string{};
            auto pos = json.find(':', key + 4);
            if (pos == std::string::npos)
                return std::string{};
            ++pos;
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
                ++pos;
            if (pos >= json.size())
                return std::string{};
            const auto begin = pos;
            if (json[pos] == '"') {
                ++pos;
                bool escaped = false;
                while (pos < json.size()) {
                    if (!escaped && json[pos] == '"') {
                        ++pos;
                        break;
                    }
                    if (!escaped && json[pos] == '\\')
                        escaped = true;
                    else
                        escaped = false;
                    ++pos;
                }
            } else {
                while (pos < json.size() && json[pos] != ',' && json[pos] != '}' &&
                       !std::isspace(static_cast<unsigned char>(json[pos])))
                    ++pos;
            }
            return json.substr(begin, pos - begin);
        }

        std::size_t extractNumber(const std::string& json, const std::string& field) {
            const std::string key = "\"" + field + "\"";
            const auto keyPos = json.find(key);
            if (keyPos == std::string::npos)
                return std::size_t{0};
            auto pos = json.find(':', keyPos + key.size());
            if (pos == std::string::npos)
                return std::size_t{0};
            ++pos;
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
                ++pos;
            const auto begin = pos;
            while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos])))
                ++pos;
            if (begin == pos)
                return std::size_t{0};
            try {
                return static_cast<std::size_t>(std::stoull(json.substr(begin, pos - begin)));
            } catch (const std::exception&) {
                return std::size_t{0};
            }
        }

        bool supportsWatchedFilesRegistration(const std::string& json) {
            const auto workspace = json.find("\"workspace\"");
            if (workspace == std::string::npos)
                return false;
            const auto watchedFiles = json.find("\"didChangeWatchedFiles\"", workspace);
            if (watchedFiles == std::string::npos)
                return false;
            const auto dynamicRegistration = json.find("\"dynamicRegistration\"", watchedFiles);
            if (dynamicRegistration == std::string::npos)
                return false;
            const auto colon = json.find(':', dynamicRegistration);
            if (colon == std::string::npos)
                return false;
            const auto value = json.find_first_not_of(" \t\r\n", colon + 1);
            return value != std::string::npos && json.compare(value, 4, "true") == 0;
        }

        void send(const std::string& payload) {
            std::cout << "Content-Length: " << payload.size() << "\r\n\r\n"
                      << payload << std::flush;
        }

        std::string jsonEscape(const std::string& value) {
            std::string escaped;
            for (const unsigned char c : value) {
                switch (c) {
                case '"':
                    escaped += "\\\"";
                    break;
                case '\\':
                    escaped += "\\\\";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                case '\b':
                    escaped += "\\b";
                    break;
                case '\f':
                    escaped += "\\f";
                    break;
                default:
                    if (c < 0x20) {
                        constexpr char hex[] = "0123456789abcdef";
                        escaped += "\\u00";
                        escaped.push_back(hex[(c >> 4) & 0x0f]);
                        escaped.push_back(hex[c & 0x0f]);
                    } else
                        escaped.push_back(static_cast<char>(c));
                    break;
                }
            }
            return escaped;
        }

        } // namespace lsp
    } // namespace driver
} // namespace nexus
