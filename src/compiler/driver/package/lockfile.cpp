#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        ParsedPackageLockfile parsePackageLockfile(const std::string& data) {
            constexpr const char* header = "# NEXUS lockfile v2 (SHA-256 content integrity)";
            ParsedPackageLockfile result;
            std::set<std::string> seenSections;
            std::istringstream input(data);
            std::string line;
            std::string activeSection;
            std::size_t lineNumber = 0;
            bool checkedHeader = false;

            while (std::getline(input, line)) {
                ++lineNumber;
                const std::string raw = trim(line);
                if (!checkedHeader && raw.empty())
                    continue;
                if (!checkedHeader) {
                    checkedHeader = true;
                    if (raw != header)
                        throw std::runtime_error(
                            "pkg verify: invalid lockfile: missing NEXUS lockfile v2 header");
                    continue;
                }

                const std::string current = trim(stripTomlInlineComment(line));
                if (current.empty())
                    continue;
                if (current.front() == '[') {
                    if (current.size() < 3 || current.back() != ']' ||
                        current.rfind("[[", 0) == 0 || current.find(']', 1) != current.size() - 1)
                        throw std::runtime_error("pkg verify: invalid lockfile table at line " +
                                                 std::to_string(lineNumber));
                    activeSection = trim(current.substr(1, current.size() - 2));
                    if (activeSection != "dependencies" && activeSection != "sources" &&
                        activeSection != "integrity")
                        throw std::runtime_error("pkg verify: unsupported lockfile table [" +
                                                 activeSection + "]");
                    if (!seenSections.insert(activeSection).second)
                        throw std::runtime_error("pkg verify: invalid lockfile: duplicate table [" +
                                                 activeSection + "]");
                    continue;
                }

                const auto equal = findTomlAssignment(current);
                if (equal == std::string::npos)
                    throw std::runtime_error("pkg verify: invalid lockfile entry at line " +
                                             std::to_string(lineNumber));
                const std::string key = trim(current.substr(0, equal));
                if (!validPackageName(key))
                    throw std::runtime_error("pkg verify: invalid lockfile key at line " +
                                             std::to_string(lineNumber));
                const std::string value = parsePackageMetadataString(
                    current.substr(equal + 1),
                    "pkg verify: invalid lockfile value at line " + std::to_string(lineNumber));

                if (activeSection.empty()) {
                    if (key != "package" && key != "version")
                        throw std::runtime_error("pkg verify: invalid lockfile metadata key '" +
                                                 key + "'");
                    if (!result.metadata.emplace(key, value).second)
                        throw std::runtime_error("pkg verify: invalid lockfile: duplicate key '" +
                                                 key + "'");
                } else if (!result.sections[activeSection].emplace(key, value).second) {
                    throw std::runtime_error("pkg verify: invalid lockfile: duplicate key '" + key +
                                             "' in [" + activeSection + "]");
                }
            }

            if (!checkedHeader || !result.metadata.contains("package") ||
                !result.metadata.contains("version") || result.metadata.at("package").empty() ||
                result.metadata.at("version").empty())
                throw std::runtime_error(
                    "pkg verify: invalid lockfile: package and version metadata are required");
            return result;
        }


        void refreshLockfile(const ProjectConfig& cfg) {
            struct Dependency {
                std::string name;
                std::string version;
                std::string source;
                std::string integrity;
            };
            std::vector<Dependency> dependencies;
            std::istringstream input(readFile(cfg.root / "nexus.toml"));
            std::string line;
            bool inDependencies = false;
            while (std::getline(input, line)) {
                const std::string t = trim(line);
                if (t.empty() || t.front() == '#')
                    continue;
                if (t.front() == '[' && t.back() == ']') {
                    inDependencies = t == "[dependencies]";
                    continue;
                }
                if (!inDependencies)
                    continue;
                const auto eq = t.find('=');
                if (eq == std::string::npos)
                    continue;
                const std::string name = trim(t.substr(0, eq));
                std::string value = trim(t.substr(eq + 1));
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    value = value.substr(1, value.size() - 2);

                Dependency dependency{name, value, {}, {}};
                const fs::path local(value);
                const fs::path normalizedLocal = local.lexically_normal();
                auto firstDeclaredComponent = local.begin();
                while (firstDeclaredComponent != local.end() &&
                       (*firstDeclaredComponent == "." || (*firstDeclaredComponent).empty())) {
                    ++firstDeclaredComponent;
                }
                const bool declaredFromVendor =
                    firstDeclaredComponent != local.end() && *firstDeclaredComponent == "vendor";
                const bool normalizedUnderVendor =
                    normalizedLocal.begin() != normalizedLocal.end() &&
                    *normalizedLocal.begin() == "vendor";
                if (!local.is_absolute() && declaredFromVendor && !normalizedUnderVendor) {
                    throw std::runtime_error("cannot lock local dependency '" + name +
                                             "': package path escapes vendor/");
                }
                if (!local.is_absolute() && normalizedUnderVendor) {
                    const fs::path dependencyRoot = (cfg.root / normalizedLocal).lexically_normal();
                    const fs::path rel = dependencyRoot.lexically_relative(cfg.root);
                    std::error_code ec;
                    const auto dependencyStatus = fs::symlink_status(dependencyRoot, ec);
                    if (ec == std::errc::no_such_file_or_directory ||
                        (!ec && dependencyStatus.type() == fs::file_type::not_found)) {
                        ec.clear();
                    } else if (ec) {
                        throw std::runtime_error("cannot inspect local dependency '" + name +
                                                 "': " + ec.message());
                    } else if (fs::is_symlink(dependencyStatus)) {
                        throw std::runtime_error("cannot lock local dependency '" + name +
                                                 "': package directory must not be a symlink");
                    } else if (fs::is_directory(dependencyStatus)) {
                        const fs::path manifestPath = dependencyRoot / "nexus.toml";
                        const auto manifestStatus = fs::symlink_status(manifestPath, ec);
                        if (ec == std::errc::no_such_file_or_directory ||
                            (!ec && manifestStatus.type() == fs::file_type::not_found)) {
                            ec.clear();
                        } else if (ec) {
                            throw std::runtime_error("cannot inspect local package manifest '" +
                                                     name + "': " + ec.message());
                        } else if (fs::is_symlink(manifestStatus)) {
                            throw std::runtime_error("cannot lock local dependency '" + name +
                                                     "': package manifest must not be a symlink");
                        } else if (fs::is_regular_file(manifestStatus)) {
                            try {
                                const fs::path canonicalRoot = fs::weakly_canonical(cfg.root, ec);
                                if (ec)
                                    throw std::runtime_error("cannot resolve project root: " +
                                                             ec.message());
                                const fs::path canonicalVendor =
                                    fs::weakly_canonical(canonicalRoot / "vendor", ec);
                                if (ec)
                                    throw std::runtime_error("cannot resolve vendor directory: " +
                                                             ec.message());
                                const fs::path vendorRelative =
                                    canonicalVendor.lexically_relative(canonicalRoot);
                                if (vendorRelative != fs::path("vendor"))
                                    throw std::runtime_error(
                                        "vendor directory resolves outside the project");
                                const fs::path canonicalDependency =
                                    fs::weakly_canonical(dependencyRoot, ec);
                                if (ec)
                                    throw std::runtime_error("cannot resolve package directory: " +
                                                             ec.message());
                                const fs::path insideVendor =
                                    canonicalDependency.lexically_relative(canonicalVendor);
                                if (insideVendor.empty() || *insideVendor.begin() == ".." ||
                                    *insideVendor.begin() == ".") {
                                    throw std::runtime_error(
                                        "local package path escapes vendor/");
                                }

                                const auto dep = loadProject(canonicalDependency);
                                if (dep.name != name)
                                    throw std::runtime_error(
                                        "package manifest name does not match dependency key '" +
                                        name + "'");
                                if (!dep.version.empty())
                                    dependency.version = dep.version;
                                dependency.source = rel.generic_string();
                                dependency.integrity = packageTreeSha256(canonicalDependency);
                            } catch (const std::exception& error) {
                                throw std::runtime_error("cannot lock local dependency '" + name +
                                                         "': " + error.what());
                            }
                        }
                    }
                }
                dependencies.push_back(std::move(dependency));
            }

            std::ostringstream lock;
            lock << "# NEXUS lockfile v2 (SHA-256 content integrity)\npackage = \"" << cfg.name
                 << "\"\nversion = \"" << cfg.version << "\"\n";
            if (!dependencies.empty()) {
                lock << "\n[dependencies]\n";
                for (const auto& dep : dependencies)
                    lock << dep.name << " = \"" << dep.version << "\"\n";

                bool haveSources = false, haveIntegrity = false;
                for (const auto& dep : dependencies) {
                    haveSources = haveSources || !dep.source.empty();
                    haveIntegrity = haveIntegrity || !dep.integrity.empty();
                }
                if (haveSources) {
                    lock << "\n[sources]\n";
                    for (const auto& dep : dependencies)
                        if (!dep.source.empty())
                            lock << dep.name << " = \"" << dep.source << "\"\n";
                }
                if (haveIntegrity) {
                    lock << "\n[integrity]\n";
                    for (const auto& dep : dependencies)
                        if (!dep.integrity.empty())
                            lock << dep.name << " = \"" << dep.integrity << "\"\n";
                }
            }
            writeTextAtomically(cfg.root / "nexus.lock", lock.str());
        }

    } // namespace driver
} // namespace nexus
