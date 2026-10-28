#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {

        namespace {

            // Project manifests feed artifact names, entry paths and window
            // settings. A truncated or hand-edited file must not silently fall
            // back to defaults, so reject structural damage while keeping the
            // accepted subset the same as the existing key/value parser.
            void validateProjectManifest(const std::string& data, const std::string& context) {

                std::set<std::string> tables;

                std::map<std::string, std::set<std::string>> keys;

                std::string current;

                std::istringstream input(data);

                std::string line;

                std::size_t lineNumber = 0;

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

                        const std::string section = trim(text.substr(1, text.size() - 2));

                        if (section.empty() || section.find('[') != std::string::npos ||
                            section.find(']') != std::string::npos)
                            throw std::runtime_error(context + ": invalid table header at line " +
                                                     std::to_string(lineNumber));

                        if (!tables.insert(section).second)
                            throw std::runtime_error(context + ": duplicate table [" + section +
                                                     "]");

                        current = section;

                        continue;
                    }

                    const std::size_t equal = findTomlAssignment(text);

                    if (equal == std::string::npos || trim(text.substr(equal + 1)).empty())
                        throw std::runtime_error(context + ": malformed entry at line " +
                                                 std::to_string(lineNumber) +
                                                 ": expected key = value");

                    const std::string key = trim(text.substr(0, equal));

                    const std::string value = trim(text.substr(equal + 1));

                    if (key.empty() || (value.front() == '"' &&
                                        (value.size() < 2 || value.back() != '"')))
                        throw std::runtime_error(context + ": malformed entry at line " +
                                                 std::to_string(lineNumber) +
                                                 ": expected key = value");

                    if (!keys[current].insert(key).second) {
                        const std::string where =
                            current.empty() ? "the manifest root" : "[" + current + "]";
                        throw std::runtime_error(context + ": duplicate key '" + key + "' in " +
                                                 where);
                    }
                }
            }

        } // namespace

        fs::path findProjectRoot(fs::path start) {

            std::error_code ec;

            start = fs::weakly_canonical(start, ec);

            if (ec)
                start = fs::absolute(start, ec);

            if (fs::is_regular_file(start, ec))
                start = start.parent_path();

            for (;;) {

                if (fs::exists(start / "nexus.toml"))
                    return start;

                const auto parent = start.parent_path();

                if (parent == start)
                    break;

                start = parent;
            }
            return {};
        }


        ProjectConfig loadProject(const fs::path& requestedRoot ) {

            const fs::path root = findProjectRoot(requestedRoot);

            if (root.empty())
                throw std::runtime_error(
                    "nexus.toml not found; run `nexus init` or pass a .nx file");

            const std::string data = readFile(root / "nexus.toml");

            validateProjectManifest(data, (root / "nexus.toml").string());

            ProjectConfig cfg;

            cfg.root = root;

            cfg.name = tomlString(data, "package", "name");

            cfg.version = tomlString(data, "package", "version");

            cfg.entry = tomlString(data, "build", "entry");

            cfg.artifact = tomlString(data, "build", "artifact");

            cfg.target = tomlString(data, "build", "target");

            cfg.windowWidth = tomlInt(data, "window", "width", 1280);

            cfg.windowHeight = tomlInt(data, "window", "height", 720);

            cfg.windowTitle = tomlString(data, "window", "title");

            if (cfg.name.empty())
                cfg.name = root.filename().string();

            if (cfg.version.empty())
                cfg.version = kVersion;

            if (cfg.entry.empty())
                cfg.entry = "src/main.nx";

            if (cfg.artifact.empty())
                cfg.artifact = cfg.name;

            if (cfg.windowTitle.empty())
                cfg.windowTitle = cfg.name;

            return cfg;
        }

    } // namespace driver
} // namespace nexus
