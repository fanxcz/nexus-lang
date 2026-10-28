#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        std::string
        tomlString(const std::string& data, const std::string& section, const std::string& key) {

            std::istringstream in(data);

            std::string line;

            std::string current;

            while (std::getline(in, line)) {

                line = trim(line);

                if (line.empty() || line[0] == '#')
                    continue;

                if (line.front() == '[' && line.back() == ']') {

                    current = trim(line.substr(1, line.size() - 2));

                    continue;
                }
                if (current != section)
                    continue;

                const auto eq = line.find('=');

                if (eq == std::string::npos)
                    continue;

                const auto lhs = trim(line.substr(0, eq));

                if (lhs != key)
                    continue;

                const auto rhs = trim(line.substr(eq + 1));

                if (rhs.size() >= 2 && rhs.front() == '"' && rhs.back() == '"')
                    return rhs.substr(1, rhs.size() - 2);

                return rhs;
            }
            return {};
        }


        int64_t tomlInt(const std::string& data,
                        const std::string& section,
                        const std::string& key,
                        int64_t fallback) {

            const std::string value = tomlString(data, section, key);

            if (value.empty())
                return fallback;

            try {
                return std::stoll(value);
            } catch (...) {
                return fallback;
            }
        }

    } // namespace driver
} // namespace nexus
