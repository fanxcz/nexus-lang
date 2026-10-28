#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        int commandFmt(const fs::path& file, bool checkOnly ) {
            const std::string src = readFile(file);
            const std::string formatted = formatNexusSource(src);
            const bool touched = formatted != src;
            if (touched && checkOnly) {
                std::cout << "fmt: needs formatting -> " << file << '\n';
                return 1;
            }
            if (touched)
                writeText(file, formatted);
            if (checkOnly)
                std::cout << "fmt: OK -> " << file << '\n';
            else
                std::cout << "fmt: " << (touched ? "updated" : "already formatted") << " -> "
                          << file << '\n';
            return 0;
        }


        int commandFmtAll(bool checkOnly) {
            const auto cfg = loadProject();
            static const std::set<std::string> excludedDirectories = {".git",
                                                                      ".vscode",
                                                                      "build",
                                                                      "dist",
                                                                      "vendor",
                                                                      "node_modules",
                                                                      "CMakeFiles",
                                                                      "target",
                                                                      ".cache"};

            std::vector<fs::path> files;
            std::error_code ec;
            for (fs::recursive_directory_iterator
                     it(cfg.root, fs::directory_options::skip_permission_denied, ec),
                 end;
                 it != end;
                 it.increment(ec)) {
                if (ec)
                    throw std::runtime_error("cannot discover NEXUS source files: " + ec.message());

                const auto status = it->symlink_status(ec);
                if (ec)
                    throw std::runtime_error("cannot inspect project entry: " + ec.message());

                if (fs::is_symlink(status)) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_directory(status) &&
                    excludedDirectories.contains(it->path().filename().string())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_regular_file(status) && it->path().extension() == ".nx")
                    files.push_back(it->path());
            }

            std::sort(files.begin(), files.end(), [](const fs::path& left, const fs::path& right) {
                return left.generic_string() < right.generic_string();
            });

            if (files.empty()) {
                std::cout << "fmt: no .nx files found in " << cfg.root << '\n';
                return 0;
            }

            std::size_t failed = 0;
            for (const auto& file : files) {
                try {
                    const int result = commandFmt(file, checkOnly);
                    if (result != 0)
                        ++failed;
                } catch (const std::exception& error) {
                    ++failed;
                    std::cerr << "fmt: " << file << ": " << error.what() << '\n';
                }
            }

            if (checkOnly) {
                if (failed != 0) {
                    std::cout << "fmt: " << failed
                              << " file(s) need formatting or could not be read\n";
                    return 1;
                }
                std::cout << "fmt: OK (" << files.size() << " file(s))\n";
                return 0;
            }

            std::cout << "fmt: processed " << files.size() << " file(s)"
                      << (failed ? ", " + std::to_string(failed) + " failure(s)" : "") << '\n';
            return failed == 0 ? 0 : 1;
        }

    } // namespace driver
} // namespace nexus
