#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {

        std::string formatDiagnostic(const std::exception& error, const std::string& code) {
            const std::string raw = error.what();
            if (raw.rfind("NEXUS found ", 0) == 0 &&
                (raw.find(" syntax error") != std::string::npos ||
                 raw.find(" semantic error") != std::string::npos)) {
                const std::string nestedCode =
                    raw.find(" semantic error") != std::string::npos ? "NX2001" : "NX1001";
                std::istringstream input(raw);
                std::string line;
                std::getline(input, line);
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
                const std::string_view location(raw.data() + marker + 4,
                                                raw.size() - marker - 4);
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

                std::string sourcePath;
                std::size_t line = 0;
                std::size_t column = 0;
                const std::size_t columnSeparator = location.rfind(':');
                const std::size_t lineSeparator =
                    columnSeparator == std::string_view::npos || columnSeparator == 0
                        ? std::string_view::npos
                        : location.rfind(':', columnSeparator - 1);
                std::size_t columnEnd = columnSeparator == std::string_view::npos
                                            ? std::string_view::npos
                                            : columnSeparator + 1;
                while (columnEnd != std::string_view::npos && columnEnd < location.size() &&
                       location[columnEnd] >= '0' && location[columnEnd] <= '9') {
                    ++columnEnd;
                }
                const auto columnText =
                    columnSeparator == std::string_view::npos
                        ? std::string_view{}
                        : location.substr(columnSeparator + 1,
                                          columnEnd - columnSeparator - 1);
                bool parsed = false;

                if (lineSeparator != std::string_view::npos &&
                    parseNumber(location.substr(lineSeparator + 1,
                                                columnSeparator - lineSeparator - 1), line) &&
                    parseNumber(columnText, column)) {
                    sourcePath.assign(location.substr(0, lineSeparator));
                    parsed = !sourcePath.empty();
                } else if (columnSeparator != std::string_view::npos &&
                           location.find(':') == columnSeparator &&
                           parseNumber(location.substr(0, columnSeparator), line) &&
                           parseNumber(columnText, column) &&
                           !gDiagnosticSource.empty()) {
                    sourcePath = gDiagnosticSource.generic_string();
                    parsed = true;
                }

                if (parsed) {
                    const std::string* sourceText = nullptr;
                    const auto sourceIt = gDiagnosticSources.find(sourcePath);
                    if (sourceIt != gDiagnosticSources.end()) {
                        sourceText = &sourceIt->second;
                    } else if (!gDiagnosticSource.empty() &&
                               gDiagnosticSource.generic_string() == sourcePath) {
                        sourceText = &gDiagnosticText;
                    }

                    std::ostringstream out;
                    out << sourcePath << ":" << line << ":" << column << ": error[" << code
                        << "]: " << trim(raw.substr(0, marker));
                    if (sourceText) {
                        std::istringstream source(*sourceText);
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
                    }
                    return out.str();
                }
            }
            return "error[NX0000]: " + raw;
        }

    } // namespace driver
} // namespace nexus
