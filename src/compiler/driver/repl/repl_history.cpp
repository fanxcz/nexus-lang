#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {

        fs::path replHistoryPath() {
            if (const char* overridePath = std::getenv("NEXUS_HISTORY_FILE")) {
                if (*overridePath != '\0')
                    return fs::path(overridePath);
            }
#ifdef _WIN32
            if (const char* appData = std::getenv("APPDATA")) {
                if (*appData != '\0')
                    return fs::path(appData) / "NEXUS" / "repl_history.txt";
            }
            if (const char* home = std::getenv("USERPROFILE")) {
                if (*home != '\0')
                    return fs::path(home) / ".nexus_repl_history";
            }
#else
            if (const char* home = std::getenv("HOME")) {
                if (*home != '\0')
                    return fs::path(home) / ".nexus_repl_history";
            }
#endif
            return {};
        }


        std::string encodeReplHistoryEntry(const std::string& entry) {
            std::string out;
            for (char ch : entry) {
                switch (ch) {
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                default:
                    out += ch;
                    break;
                }
            }
            return out;
        }


        std::string decodeReplHistoryEntry(const std::string& encoded) {
            std::string out;
            for (std::size_t i = 0; i < encoded.size(); ++i) {
                if (encoded[i] != '\\' || i + 1 >= encoded.size()) {
                    out += encoded[i];
                    continue;
                }
                const char ch = encoded[++i];
                if (ch == 'n') {
                    out += '\n';
                } else if (ch == 'r') {
                    out += '\r';
                } else if (ch == '\\') {
                    out += '\\';
                } else {
                    out += '\\';
                    out += ch;
                }
            }
            return out;
        }


        std::vector<std::string> loadReplHistory(const fs::path& path) {
            std::vector<std::string> entries;
            if (path.empty())
                return entries;

            std::ifstream in(path, std::ios::binary);
            std::string line;
            while (std::getline(in, line)) {
                entries.push_back(decodeReplHistoryEntry(line));
                if (entries.size() > kReplHistoryLimit)
                    entries.erase(entries.begin());
            }
            return entries;
        }


        bool saveReplHistory(const fs::path& path, const std::vector<std::string>& entries) {
            if (path.empty())
                return true;

            try {
                std::ostringstream serialized;
                for (const auto& entry : entries)
                    serialized << encodeReplHistoryEntry(entry) << '\n';

                // Keep the last valid history file if the process is interrupted
                // while writing the next version.
                writeTextAtomically(path, serialized.str());
                return true;
            } catch (const std::exception&) {
                return false;
            }
        }

    } // namespace driver
} // namespace nexus
