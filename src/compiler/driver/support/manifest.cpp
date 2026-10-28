#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {

        // Single manifest reader shared by the project commands (check, build,
        // info, run) and the package commands (pkg add/remove/update/verify/list).
        // Keeping one implementation is what stops the two command families from
        // accepting different files.
        //
        // Accepted subset: `[table]` headers, `key = value` entries with bare or
        // double-quoted values. Comments start with `#` outside a string. Table
        // names are unique and keys are unique within their table.
        //
        // Errors: throws std::runtime_error with `<context>: <reason> at line <n>`
        // for structural damage; `context` is the manifest path.
        std::vector<ManifestEntry> parseManifestEntries(const std::string& data,
                                                        const std::string& context) {
            std::vector<ManifestEntry> entries;
            std::set<std::string> tables;
            std::set<std::pair<std::string, std::string>> keys;
            std::string section;
            std::size_t lineNumber = 0;
            std::istringstream input(data);
            std::string line;

            while (std::getline(input, line)) {
                ++lineNumber;
                const std::string text = trim(stripTomlInlineComment(line));

                if (text.empty())
                    continue;

                if (text.front() == '[') {
                    const bool regularTable = text.size() >= 3 && text.back() == ']' &&
                                              text.rfind("[[", 0) != 0 &&
                                              text.find(']') == text.size() - 1;

                    if (!regularTable)
                        throw std::runtime_error(context + ": invalid table header at line " +
                                                 std::to_string(lineNumber));

                    const std::string name = trim(text.substr(1, text.size() - 2));

                    if (name.empty() || name.find('[') != std::string::npos ||
                        name.find(']') != std::string::npos)
                        throw std::runtime_error(context + ": invalid table header at line " +
                                                 std::to_string(lineNumber));

                    if (!tables.insert(name).second)
                        throw std::runtime_error(context + ": duplicate table [" + name + "]");

                    section = name;
                    continue;
                }

                const std::size_t equal = findTomlAssignment(text);

                if (equal == std::string::npos || trim(text.substr(equal + 1)).empty())
                    throw std::runtime_error(context + ": malformed entry at line " +
                                             std::to_string(lineNumber) + ": expected key = value");

                const std::string key = trim(text.substr(0, equal));
                const std::string value = trim(text.substr(equal + 1));

                if (key.empty() ||
                    (value.front() == '"' && (value.size() < 2 || value.back() != '"')))
                    throw std::runtime_error(context + ": malformed entry at line " +
                                             std::to_string(lineNumber) + ": expected key = value");

                if (!keys.emplace(section, key).second) {
                    const std::string where =
                        section.empty() ? "the manifest root" : "[" + section + "]";
                    throw std::runtime_error(context + ": duplicate key '" + key + "' in " + where);
                }

                entries.push_back(ManifestEntry{section, key, value, lineNumber});
            }

            return entries;
        }

    } // namespace driver
} // namespace nexus
