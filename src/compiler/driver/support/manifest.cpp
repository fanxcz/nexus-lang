#include "driver/driver_internal.hpp"

#include <tuple>

namespace nexus {
    namespace driver {

        namespace {

            // Tracks bracket depth and string state of an array value. Arrays may
            // span lines, so the state has to survive from one line to the next.
            struct ValueScan {
                int depth = 0;
                bool inString = false;
                bool escaped = false;
            };

            void scanValue(const std::string& text, ValueScan& scan) {
                for (const char ch : text) {
                    if (scan.inString) {
                        if (scan.escaped)
                            scan.escaped = false;
                        else if (ch == '\\')
                            scan.escaped = true;
                        else if (ch == '"')
                            scan.inString = false;
                        continue;
                    }
                    if (ch == '"')
                        scan.inString = true;
                    else if (ch == '[')
                        ++scan.depth;
                    else if (ch == ']')
                        --scan.depth;
                }
            }

        } // namespace

        // Single manifest reader shared by the project commands (check, build,
        // info, run) and the package commands (pkg add/remove/update/verify/list).
        // Keeping one implementation is what stops the two command families from
        // accepting different files.
        //
        // Accepted subset: `[table]` and `[[array-of-tables]]` headers, and
        // `key = value` entries whose value is a bare scalar, a double-quoted
        // string, or an array of those (arrays may span lines, nest, contain
        // comments and use a trailing comma). Comments start with `#` outside a
        // string. Regular table names are unique; array-of-tables headers may
        // repeat; keys are unique within one table instance.
        //
        // Arrays are parsed structurally only: they are not exposed through the
        // scalar accessors (tomlString/tomlInt), which report them as absent.
        //
        // Errors: throws std::runtime_error with `<context>: <reason> at line <n>`
        // for structural damage; `context` is the manifest path.
        std::vector<ManifestEntry> parseManifestEntries(const std::string& data,
                                                        const std::string& context) {
            std::vector<ManifestEntry> entries;
            std::set<std::string> tables;
            std::map<std::string, std::size_t> arrayTables;
            std::set<std::tuple<std::string, std::size_t, std::string>> keys;
            std::string section;
            std::size_t instance = 0;
            std::size_t lineNumber = 0;
            std::istringstream input(data);
            std::string line;

            while (std::getline(input, line)) {
                ++lineNumber;
                const std::string text = trim(stripTomlInlineComment(line));

                if (text.empty())
                    continue;

                if (text.front() == '[') {
                    const bool arrayTable = text.size() >= 4 && text.rfind("[[", 0) == 0 &&
                                            text.substr(text.size() - 2) == "]]";
                    const bool regularTable = !arrayTable && text.size() >= 3 &&
                                              text.back() == ']' &&
                                              text.find(']') == text.size() - 1;

                    if (!arrayTable && !regularTable)
                        throw std::runtime_error(context + ": invalid table header at line " +
                                                 std::to_string(lineNumber));

                    const std::size_t start = arrayTable ? 2 : 1;
                    const std::size_t length = text.size() - start - (arrayTable ? 2 : 1);
                    const std::string name = trim(text.substr(start, length));

                    if (name.empty() || name.find('[') != std::string::npos ||
                        name.find(']') != std::string::npos)
                        throw std::runtime_error(context + ": invalid table header at line " +
                                                 std::to_string(lineNumber));

                    if (arrayTable) {
                        if (tables.contains(name))
                            throw std::runtime_error(context + ": duplicate table [" + name + "]");
                        instance = ++arrayTables[name];
                    } else {
                        if (arrayTables.contains(name) || !tables.insert(name).second)
                            throw std::runtime_error(context + ": duplicate table [" + name + "]");
                        instance = 0;
                    }

                    section = name;
                    continue;
                }

                const std::size_t equal = findTomlAssignment(text);

                if (equal == std::string::npos || trim(text.substr(equal + 1)).empty())
                    throw std::runtime_error(context + ": malformed entry at line " +
                                             std::to_string(lineNumber) + ": expected key = value");

                const std::string key = trim(text.substr(0, equal));
                std::string value = trim(text.substr(equal + 1));
                const std::size_t valueLine = lineNumber;

                if (key.empty())
                    throw std::runtime_error(context + ": malformed entry at line " +
                                             std::to_string(lineNumber) + ": expected key = value");

                if (value.front() == '[') {
                    ValueScan scan;
                    scanValue(value, scan);

                    while (scan.depth > 0) {
                        std::string continuation;

                        if (!std::getline(input, continuation))
                            throw std::runtime_error(context +
                                                     ": unterminated array value at line " +
                                                     std::to_string(valueLine));

                        ++lineNumber;

                        if (scan.inString)
                            throw std::runtime_error(
                                context + ": unterminated string in array value at line " +
                                std::to_string(valueLine));

                        const std::string stripped = trim(stripTomlInlineComment(continuation));

                        if (stripped.empty())
                            continue;

                        value += " ";
                        value += stripped;
                        scanValue(stripped, scan);

                        if (scan.depth < 0)
                            throw std::runtime_error(context + ": malformed array value at line " +
                                                     std::to_string(valueLine));
                    }

                    if (scan.inString)
                        throw std::runtime_error(context +
                                                 ": unterminated string in array value at line " +
                                                 std::to_string(valueLine));
                } else if (value.front() == '"' && (value.size() < 2 || value.back() != '"')) {
                    throw std::runtime_error(context + ": malformed entry at line " +
                                             std::to_string(lineNumber) + ": expected key = value");
                }

                if (!keys.emplace(section, instance, key).second) {
                    const std::string where =
                        section.empty() ? "the manifest root" : "[" + section + "]";
                    throw std::runtime_error(context + ": duplicate key '" + key + "' in " + where);
                }

                entries.push_back(ManifestEntry{section, key, value, valueLine, instance});
            }

            return entries;
        }

    } // namespace driver
} // namespace nexus
