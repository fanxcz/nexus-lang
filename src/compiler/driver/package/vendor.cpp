#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        void ensureSafeVendorDirectory(const ProjectConfig& cfg, bool createIfMissing) {
            const fs::path vendor = cfg.root / "vendor";
            std::error_code ec;
            const auto status = fs::symlink_status(vendor, ec);
            if (ec == std::errc::no_such_file_or_directory ||
                (!ec && status.type() == fs::file_type::not_found)) {
                ec.clear();
                if (createIfMissing) {
                    fs::create_directories(vendor, ec);
                    if (ec)
                        throw std::runtime_error("cannot create vendor directory: " + ec.message());
                }
                return;
            }
            if (ec)
                throw std::runtime_error("cannot inspect vendor directory: " + ec.message());
            if (fs::is_symlink(status))
                throw std::runtime_error("vendor directory must not be a symlink");
            if (!fs::is_directory(status))
                throw std::runtime_error("vendor path exists but is not a directory");

            const fs::path canonicalRoot = fs::weakly_canonical(cfg.root, ec);
            if (ec)
                throw std::runtime_error("cannot resolve project root: " + ec.message());
            const fs::path canonicalVendor = fs::canonical(vendor, ec);
            if (ec)
                throw std::runtime_error("cannot resolve vendor directory: " + ec.message());
            const fs::path relativeVendor = canonicalVendor.lexically_relative(canonicalRoot);
            if (relativeVendor != fs::path("vendor"))
                throw std::runtime_error("vendor directory resolves outside the project root");
        }


        void updateDependency(const fs::path& manifest,
                              const std::string& name,
                              const std::string& value,
                              bool remove) {
            std::istringstream input(readFile(manifest));
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(input, line))
                lines.push_back(line);
            std::size_t start = lines.size(), finish = lines.size();
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (trim(lines[i]) != "[dependencies]")
                    continue;
                start = i + 1;
                for (std::size_t j = i + 1; j < lines.size(); ++j) {
                    const std::string t = trim(lines[j]);
                    if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
                        finish = j;
                        break;
                    }
                }
                break;
            }
            if (start == lines.size()) {
                if (remove)
                    return;
                if (!lines.empty() && !trim(lines.back()).empty())
                    lines.emplace_back();
                lines.emplace_back("[dependencies]");
                lines.emplace_back(name + " = \"" + value + "\"");
            } else {
                bool found = false;
                std::size_t at = start;
                for (; at < finish; ++at) {
                    const std::string t = trim(lines[at]);
                    if (t.empty() || t.front() == '#')
                        continue;
                    const auto eq = t.find('=');
                    if (eq == std::string::npos || trim(t.substr(0, eq)) != name)
                        continue;
                    found = true;
                    break;
                }
                if (found) {
                    if (remove)
                        lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(at));
                    else
                        lines[at] = name + " = \"" + value + "\"";
                } else if (!remove) {
                    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(finish),
                                 name + " = \"" + value + "\"");
                }
            }
            std::ostringstream output;
            for (const auto& entry : lines)
                output << entry << '\n';
            writeTextAtomically(manifest, output.str());
        }


        void copyPackageTree(const fs::path& source, const fs::path& destination) {
            const std::set<std::string> skipped = {
                ".git", ".vscode", "build", "dist", "node_modules", "CMakeFiles"};
            std::error_code ec;
            fs::create_directories(destination, ec);
            if (ec)
                throw std::runtime_error("cannot create vendor directory: " + ec.message());
            try {
                for (fs::recursive_directory_iterator
                         it(source, fs::directory_options::skip_permission_denied, ec),
                     end;
                     it != end;
                     it.increment(ec)) {
                    if (ec)
                        throw std::runtime_error("cannot traverse package source: " + ec.message());
                    const auto status = it->symlink_status(ec);
                    if (ec)
                        throw std::runtime_error("cannot inspect package entry: " + ec.message());
                    if (fs::is_symlink(status))
                        continue;
                    if (fs::is_directory(status) &&
                        skipped.contains(it->path().filename().string())) {
                        it.disable_recursion_pending();
                        continue;
                    }
                    const fs::path rel = it->path().lexically_relative(source);
                    const fs::path target = destination / rel;
                    if (fs::is_directory(status))
                        fs::create_directories(target, ec);
                    else if (fs::is_regular_file(status)) {
                        fs::create_directories(target.parent_path(), ec);
                        if (!ec)
                            fs::copy_file(
                                it->path(), target, fs::copy_options::overwrite_existing, ec);
                    }
                    if (ec)
                        throw std::runtime_error("cannot copy package entry '" + rel.string() +
                                                 "': " + ec.message());
                }
            } catch (...) {
                std::error_code cleanup;
                fs::remove_all(destination, cleanup);
                throw;
            }
        }

    } // namespace driver
} // namespace nexus
