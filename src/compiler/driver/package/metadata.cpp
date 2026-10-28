#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {

        bool validPackageName(const std::string& name) {
            if (name.empty() || name == "." || name == "..")
                return false;
            for (const unsigned char ch : name) {
                if (!std::isalnum(ch) && ch != '_' && ch != '-')
                    return false;
            }
            return true;
        }

        std::string stripTomlInlineComment(const std::string& line) {
            bool inString = false;
            bool escaped = false;
            for (std::size_t i = 0; i < line.size(); ++i) {
                const char ch = line[i];
                if (inString) {
                    if (escaped)
                        escaped = false;
                    else if (ch == '\\')
                        escaped = true;
                    else if (ch == '"')
                        inString = false;
                } else if (ch == '"') {
                    inString = true;
                } else if (ch == '#') {
                    return line.substr(0, i);
                }
            }
            return line;
        }

        std::size_t findTomlAssignment(const std::string& line) {
            bool inString = false;
            bool escaped = false;
            for (std::size_t i = 0; i < line.size(); ++i) {
                const char ch = line[i];
                if (inString) {
                    if (escaped)
                        escaped = false;
                    else if (ch == '\\')
                        escaped = true;
                    else if (ch == '"')
                        inString = false;
                } else if (ch == '"') {
                    inString = true;
                } else if (ch == '=') {
                    return i;
                }
            }
            return std::string::npos;
        }

        std::string parsePackageMetadataString(const std::string& rawValue,
                                               const std::string& context) {
            const std::string value = trim(rawValue);
            if (value.size() < 2 || value.front() != '"' || value.back() != '"')
                throw std::runtime_error(context + ": expected a quoted string");

            std::string decoded;
            decoded.reserve(value.size() - 2);
            for (std::size_t i = 1; i + 1 < value.size(); ++i) {
                const char ch = value[i];
                if (ch == '"')
                    throw std::runtime_error(context + ": unescaped quote in string");
                if (ch != '\\') {
                    if (static_cast<unsigned char>(ch) < 0x20)
                        throw std::runtime_error(context + ": control character in string");
                    decoded.push_back(ch);
                    continue;
                }

                ++i;
                if (i + 1 >= value.size())
                    throw std::runtime_error(context + ": unfinished string escape");
                switch (value[i]) {
                case '"':
                    decoded.push_back('"');
                    break;
                case '\\':
                    decoded.push_back('\\');
                    break;
                case 'b':
                    decoded.push_back('\b');
                    break;
                case 't':
                    decoded.push_back('\t');
                    break;
                case 'n':
                    decoded.push_back('\n');
                    break;
                case 'f':
                    decoded.push_back('\f');
                    break;
                case 'r':
                    decoded.push_back('\r');
                    break;
                default:
                    throw std::runtime_error(context + ": unsupported string escape");
                }
            }
            return decoded;
        }

        std::map<std::string, std::string> readStrictTomlStringSection(
            const std::string& data, const std::string& wantedSection, const std::string& context) {
            std::map<std::string, std::string> values;
            for (const auto& entry : parseManifestEntries(data, context)) {
                if (entry.section != wantedSection)
                    continue;
                if (!validPackageName(entry.key))
                    throw std::runtime_error(context + ": invalid dependency name at line " +
                                             std::to_string(entry.line));
                const std::string value = parsePackageMetadataString(
                    entry.value,
                    context + " [" + wantedSection + "] line " + std::to_string(entry.line));
                if (!values.emplace(entry.key, value).second)
                    throw std::runtime_error(context + ": duplicate key '" + entry.key + "' in [" +
                                             wantedSection + "]");
            }
            return values;
        }

    } // namespace driver
} // namespace nexus
