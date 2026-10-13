#include "formatter.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace nexus {
    namespace {

        std::string trimLine(const std::string& line) {
            const auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                return {};
            const auto last = line.find_last_not_of(" \t\r\n");
            return line.substr(first, last - first + 1);
        }

    } // namespace

    std::string formatNexusSource(const std::string& source) {
        std::istringstream in(source);
        std::ostringstream out;
        std::string line;
        int indent = 0;

        while (std::getline(in, line)) {
            while (!line.empty() &&
                   (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
                line.pop_back();
            }
            const std::string trimmed = trimLine(line);
            if (trimmed.empty()) {
                out << '\n';
                continue;
            }
            if (trimmed.front() == '}')
                indent = std::max(0, indent - 1);
            for (int i = 0; i < indent; ++i)
                out << "    ";
            out << trimmed << '\n';
            if (trimmed.back() == '{' && trimmed.rfind("//", 0) != 0)
                ++indent;
            if (trimmed.find("} else {") != std::string::npos)
                indent = std::max(0, indent);
        }

        return out.str();
    }

} // namespace nexus
