#include "driver/lsp/protocol.hpp"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>

namespace nexus {
    namespace driver {
        namespace lsp {

        namespace {
            std::size_t findJsonFieldValueStart(const std::string& json, const std::string& field,
                                                std::size_t begin = 0,
                                                std::size_t end = std::string::npos) {
                end = std::min(end, json.size());
                std::size_t position = begin;
                while (position < end) {
                    if (json[position] != '"') {
                        ++position;
                        continue;
                    }

                    const std::size_t keyBegin = position + 1;
                    ++position;
                    bool escaped = false;
                    while (position < end) {
                        const char current = json[position];
                        if (!escaped && current == '"')
                            break;
                        if (!escaped && current == '\\')
                            escaped = true;
                        else
                            escaped = false;
                        ++position;
                    }
                    if (position >= end)
                        return std::string::npos;

                    const bool matches =
                        position - keyBegin == field.size() &&
                        json.compare(keyBegin, field.size(), field) == 0;
                    std::size_t value = position + 1;
                    while (value < end &&
                           std::isspace(static_cast<unsigned char>(json[value])))
                        ++value;
                    if (matches && value < end && json[value] == ':') {
                        ++value;
                        while (value < end &&
                               std::isspace(static_cast<unsigned char>(json[value])))
                            ++value;
                        return value;
                    }
                    ++position;
                }
                return std::string::npos;
            }

            std::size_t matchingJsonObjectEnd(const std::string& json, std::size_t begin,
                                              std::size_t end = std::string::npos) {
                end = std::min(end, json.size());
                if (begin >= end || json[begin] != '{')
                    return std::string::npos;
                std::size_t depth = 0;
                bool inString = false;
                bool escaped = false;
                for (std::size_t i = begin; i < end; ++i) {
                    const char current = json[i];
                    if (inString) {
                        if (!escaped && current == '"')
                            inString = false;
                        if (!escaped && current == '\\')
                            escaped = true;
                        else
                            escaped = false;
                        continue;
                    }
                    if (current == '"') {
                        inString = true;
                    } else if (current == '{') {
                        ++depth;
                    } else if (current == '}') {
                        if (depth == 0)
                            return std::string::npos;
                        --depth;
                        if (depth == 0)
                            return i;
                    }
                }
                return std::string::npos;
            }

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
            std::size_t pos = findJsonFieldValueStart(json, field);
            if (pos == std::string::npos)
                return std::string{};
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
            auto pos = findJsonFieldValueStart(json, "id");
            if (pos == std::string::npos)
                return std::string{};
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
            const std::size_t pos = findJsonFieldValueStart(json, field);
            if (pos == std::string::npos)
                return std::size_t{0};
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
            const std::size_t workspaceValue =
                findJsonFieldValueStart(json, "workspace");
            if (workspaceValue == std::string::npos)
                return false;
            const std::size_t workspaceEnd = matchingJsonObjectEnd(json, workspaceValue);
            if (workspaceEnd == std::string::npos)
                return false;

            const std::size_t watchedValue =
                findJsonFieldValueStart(json, "didChangeWatchedFiles", workspaceValue,
                                        workspaceEnd);
            if (watchedValue == std::string::npos)
                return false;
            const std::size_t watchedEnd =
                matchingJsonObjectEnd(json, watchedValue, workspaceEnd);
            if (watchedEnd == std::string::npos)
                return false;

            const std::size_t registrationValue =
                findJsonFieldValueStart(json, "dynamicRegistration", watchedValue,
                                        watchedEnd);
            if (registrationValue == std::string::npos ||
                json.compare(registrationValue, 4, "true") != 0)
                return false;
            const std::size_t afterTrue = registrationValue + 4;
            return afterTrue == watchedEnd || json[afterTrue] == ',' ||
                   json[afterTrue] == '}' ||
                   std::isspace(static_cast<unsigned char>(json[afterTrue]));

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
