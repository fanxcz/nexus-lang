#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        std::string readFile(const fs::path& p) {

            std::ifstream f(p, std::ios::binary);

            if (!f)
                throw std::runtime_error("cannot open " + p.string());

            return {std::istreambuf_iterator<char>(f), {}};
        }


        std::string trim(std::string s) {

            const auto first = s.find_first_not_of(" \t\r\n");

            if (first == std::string::npos)
                return {};

            const auto last = s.find_last_not_of(" \t\r\n");

            return s.substr(first, last - first + 1);
        }


        void writeText(const fs::path& p, const std::string& data) {

            fs::create_directories(p.parent_path().empty() ? fs::current_path() : p.parent_path());

            std::ofstream f(p, std::ios::binary);

            if (!f)
                throw std::runtime_error("cannot write " + p.string());

            f << data;
        }


        // Write to a sibling temporary file and replace the destination only after
        // a successful flush. An interrupted write must not truncate package metadata.
        void writeTextAtomically(const fs::path& p, const std::string& data) {
            const fs::path parent = p.parent_path().empty() ? fs::current_path() : p.parent_path();
            fs::create_directories(parent);
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            fs::path temporary = p;
            temporary += ".tmp-" + std::to_string(stamp);

            {
                std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
                if (!out)
                    throw std::runtime_error("cannot create temporary metadata file " +
                                             temporary.string());
                out.write(data.data(), static_cast<std::streamsize>(data.size()));
                out.flush();
                if (!out) {
                    out.close();
                    std::error_code cleanup;
                    fs::remove(temporary, cleanup);
                    throw std::runtime_error("cannot finish writing temporary metadata file " +
                                             temporary.string());
                }
            }

#ifdef _WIN32
            if (!MoveFileExW(temporary.c_str(),
                             p.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                const DWORD error = GetLastError();
                std::error_code cleanup;
                fs::remove(temporary, cleanup);
                throw std::runtime_error("cannot atomically replace " + p.string() +
                                         " (Windows error " + std::to_string(error) + ")");
            }
#else
            std::error_code ec;
            fs::rename(temporary, p, ec);
            if (ec) {
                std::error_code cleanup;
                fs::remove(temporary, cleanup);
                throw std::runtime_error("cannot atomically replace " + p.string() + ": " +
                                         ec.message());
            }
#endif
        }


        std::string packageTreeSha256(const fs::path& root) {
            static const std::set<std::string> excluded = {
                ".git", ".vscode", "build", "dist", "node_modules", "CMakeFiles"};
            std::vector<fs::path> files;
            std::error_code ec;
            for (fs::recursive_directory_iterator
                     it(root, fs::directory_options::skip_permission_denied, ec),
                 end;
                 it != end;
                 it.increment(ec)) {
                if (ec)
                    throw std::runtime_error("cannot traverse package for checksum: " +
                                             ec.message());
                const auto status = it->symlink_status(ec);
                if (ec)
                    throw std::runtime_error("cannot inspect package for checksum: " +
                                             ec.message());
                if (fs::is_symlink(status)) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_directory(status) && excluded.contains(it->path().filename().string())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_regular_file(status))
                    files.push_back(it->path());
            }

            std::sort(files.begin(), files.end(), [&](const fs::path& left, const fs::path& right) {
                return left.lexically_relative(root).generic_string() <
                       right.lexically_relative(root).generic_string();
            });
            std::string manifest;
            for (const auto& file : files) {
                manifest += file.lexically_relative(root).generic_string();
                manifest.push_back('\0');
                manifest += package::sha256Hex(readFile(file));
                manifest.push_back('\n');
            }
            return "sha256:" + package::sha256Hex(manifest);
        }

    } // namespace driver
} // namespace nexus
