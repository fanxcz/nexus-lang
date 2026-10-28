#include "driver/lsp/uri.hpp"

#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace nexus {
    namespace driver {
        namespace lsp {

        std::filesystem::path pathFromUri(const std::string& uri) {
            if (uri.rfind("file://", 0) != 0)
                return {};
            const std::string encoded = uri.substr(7);
            std::string decoded;
            decoded.reserve(encoded.size());
            const auto hexValue = [](char c) -> int {
                if (c >= '0' && c <= '9')
                    return c - '0';
                if (c >= 'a' && c <= 'f')
                    return c - 'a' + 10;
                if (c >= 'A' && c <= 'F')
                    return c - 'A' + 10;
                return -1;
            };
            for (std::size_t i = 0; i < encoded.size(); ++i) {
                if (encoded[i] == '%' && i + 2 < encoded.size()) {
                    const int high = hexValue(encoded[i + 1]);
                    const int low = hexValue(encoded[i + 2]);
                    if (high >= 0 && low >= 0) {
                        decoded.push_back(static_cast<char>((high << 4) | low));
                        i += 2;
                        continue;
                    }
                }
                decoded.push_back(encoded[i]);
            }
#ifdef _WIN32
            if (decoded.size() >= 3 && decoded[0] == '/' &&
                ((decoded[1] >= 'A' && decoded[1] <= 'Z') ||
                 (decoded[1] >= 'a' && decoded[1] <= 'z')) &&
                decoded[2] == ':') {
                decoded.erase(decoded.begin());
            }
#endif
            return fs::path(decoded);
        }

        std::string uriFromPath(const std::filesystem::path& inputPath) {
            std::error_code ec;
            fs::path absolute = fs::absolute(inputPath, ec);
            if (ec)
                absolute = inputPath;
            std::string path = absolute.lexically_normal().generic_string();
            if (path.empty() || path.front() != '/')
                path.insert(path.begin(), '/');
            static constexpr char hex[] = "0123456789ABCDEF";
            std::string encoded;
            encoded.reserve(path.size());
            for (const unsigned char ch : path) {
                const bool unreserved = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                                        (ch >= '0' && ch <= '9') || ch == '/' || ch == ':' ||
                                        ch == '-' || ch == '_' || ch == '.' || ch == '~';
                if (unreserved) {
                    encoded.push_back(static_cast<char>(ch));
                } else {
                    encoded.push_back('%');
                    encoded.push_back(hex[ch >> 4]);
                    encoded.push_back(hex[ch & 0x0f]);
                }
            }
            return std::string("file://") + encoded;
        }

        } // namespace lsp
    } // namespace driver
} // namespace nexus
