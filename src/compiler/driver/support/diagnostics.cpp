#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        std::string formatDiagnostic(const std::exception& error,
                                     const std::string& code ) {
            const std::string raw = error.what();
            if (raw.rfind("NEXUS found ", 0) == 0 &&
                (raw.find(" syntax error") != std::string::npos ||
                 raw.find(" semantic error") != std::string::npos)) {
                const std::string nestedCode =
                    raw.find(" semantic error") != std::string::npos ? "NX2001" : "NX1001";
                std::istringstream input(raw);
                std::string line;
                std::getline(input, line); // The first line is the summary count.
                std::ostringstream combined;
                bool first = true;
                while (std::getline(input, line)) {
                    if (line.rfind("  - ", 0) != 0)
                        continue;
                    const std::runtime_error one(line.substr(4));
                    if (!first)
                        combined << '\n';
                    combined << formatDiagnostic(one, nestedCode);
                    first = false;
                }
                if (!first)
                    return combined.str();
            }
            const auto marker = raw.rfind(" at ");
            if (marker != std::string::npos) {
                const std::string_view location(raw.data() + marker + 4, raw.size() - marker - 4);
                const auto separator = location.find(':');
                if (separator != std::string_view::npos &&
                    location.find(':', separator + 1) == std::string_view::npos) {
                    const auto parseNumber = [](std::string_view value, std::size_t& result) {
                        if (value.empty())
                            return false;
                        result = 0;
                        for (const char c : value) {
                            if (c < '0' || c > '9')
                                return false;
                            result = result * 10 + static_cast<std::size_t>(c - '0');
                        }
                        return result > 0;
                    };
                    std::size_t line = 0;
                    std::size_t column = 0;
                    std::size_t columnEnd = separator + 1;
                    while (columnEnd < location.size() && location[columnEnd] >= '0' &&
                           location[columnEnd] <= '9') {
                        ++columnEnd;
                    }
                    if (parseNumber(location.substr(0, separator), line) &&
                        parseNumber(location.substr(separator + 1, columnEnd - separator - 1),
                                    column) &&
                        !gDiagnosticSource.empty()) {
                        std::ostringstream out;
                        out << gDiagnosticSource.string() << ":" << line << ":" << column
                            << ": error[" << code << "]: " << trim(raw.substr(0, marker));
                        std::istringstream source(gDiagnosticText);
                        std::string sourceLine;
                        for (std::size_t current = 1;
                             current <= line && std::getline(source, sourceLine);
                             ++current) {
                            if (current == line) {
                                const std::string label = "  " + std::to_string(line) + " | ";
                                out << "\n"
                                    << label << sourceLine << "\n"
                                    << std::string(label.size(), ' ')
                                    << std::string(column - 1, ' ') << "^";
                                break;
                            }
                        }
                        return out.str();
                    }
                }
            }
            return "error[NX0000]: " + raw;
        }

    } // namespace driver
} // namespace nexus
