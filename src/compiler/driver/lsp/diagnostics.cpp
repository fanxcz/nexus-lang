#include "driver/lsp/diagnostics.hpp"

#include "driver/driver_internal.hpp"
#include "driver/lsp/text.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nexus {
    namespace driver {
        namespace lsp {

std::vector<LspDiagnostic> collectDiagnostics(const std::string& source) {
            std::vector<LspDiagnostic> diagnostics;
            const auto add = [&](const std::exception& error, const std::string& code) {
                LspDiagnostic diagnostic;
                diagnostic.code = code;
                const std::string raw = error.what();
                const auto marker = raw.rfind(" at ");
                diagnostic.message =
                    marker == std::string::npos ? raw : trim(raw.substr(0, marker));
                if (marker != std::string::npos) {
                    std::size_t line = 0, column = 0;
                    if (std::sscanf(raw.c_str() + marker + 4, "%zu:%zu", &line, &column) == 2 &&
                        line > 0 && column > 0) {
                        diagnostic.line = line - 1;
                        diagnostic.character = column - 1;
                    } else {
                        diagnostic.message = raw;
                    }
                }
                diagnostics.push_back(std::move(diagnostic));
            };

            std::vector<Token> tokens;
            try {
                tokens = Lexer(source).tokenize();
            } catch (const std::exception& error) {
                add(error, "NX1000");
                return diagnostics;
            }
            const std::vector<Token> diagnosticTokens = tokens;
            const auto applyTokenRanges = [&]() {
                for (auto& diagnostic : diagnostics) {
                    std::string target;
                    const auto quote = diagnostic.message.find('\'');
                    if (diagnostic.message.rfind("unknown variable: ", 0) == 0) {
                        target =
                            diagnostic.message.substr(std::string("unknown variable: ").size());
                    } else if (diagnostic.message.find("field") != std::string::npos &&
                               quote != std::string::npos) {
                        const auto endQuote = diagnostic.message.find('\'', quote + 1);
                        if (endQuote != std::string::npos && endQuote > quote + 1)
                            target = diagnostic.message.substr(quote + 1, endQuote - quote - 1);
                    } else if (diagnostic.message.rfind("unknown enum variant: ", 0) == 0) {
                        const auto separator = diagnostic.message.rfind("::");
                        if (separator != std::string::npos)
                            target = diagnostic.message.substr(separator + 2);
                    }
                    if (!target.empty()) {
                        const Token* closest = nullptr;
                        std::size_t bestDistance = std::numeric_limits<std::size_t>::max();
                        for (const auto& token : diagnosticTokens) {
                            if (token.kind != TokenKind::Identifier || token.text != target)
                                continue;
                            const std::size_t distance =
                                token.pos.line == diagnostic.line + 1
                                    ? (token.pos.column > diagnostic.character + 1
                                           ? token.pos.column - diagnostic.character - 1
                                           : diagnostic.character + 1 - token.pos.column)
                                    : std::numeric_limits<std::size_t>::max() / 2 +
                                          (token.pos.line > diagnostic.line + 1
                                               ? token.pos.line - diagnostic.line - 1
                                               : diagnostic.line + 1 - token.pos.line);
                            if (distance < bestDistance) {
                                bestDistance = distance;
                                closest = &token;
                            }
                        }
                        if (closest) {
                            diagnostic.line = closest->pos.line - 1;
                            diagnostic.character = closest->pos.column - 1;
                        }
                    }
                    for (const auto& token : diagnosticTokens) {
                        if (token.text.empty() || token.kind == TokenKind::End ||
                            token.pos.line != diagnostic.line + 1 ||
                            token.pos.column != diagnostic.character + 1)
                            continue;
                        diagnostic.length = std::max<std::size_t>(1, utf16Length(token.text));
                        const std::size_t offset = std::min(token.pos.offset, source.size());
                        const auto newline = source.rfind('\n', offset == 0 ? 0 : offset - 1);
                        const std::size_t lineStart =
                            newline == std::string::npos ? 0 : newline + 1;
                        diagnostic.character =
                            utf16Length(source.substr(lineStart, offset - lineStart));
                        break;
                    }
                }
            };
            Program program;
            try {
                program = Parser(std::move(tokens)).parseProgram();
            } catch (const std::exception& error) {
                const std::string message = error.what();
                if (message.rfind("NEXUS found ", 0) == 0 &&
                    message.find(" syntax error") != std::string::npos) {
                    std::istringstream errors(message);
                    std::string line;
                    std::getline(errors, line); // Skip the aggregate summary.
                    while (std::getline(errors, line)) {
                        if (line.rfind("  - ", 0) != 0)
                            continue;
                        const std::runtime_error one(line.substr(4));
                        add(one, "NX1001");
                    }
                    if (diagnostics.empty())
                        add(error, "NX1001");
                } else {
                    add(error, "NX1001");
                }
                applyTokenRanges();
                return diagnostics;
            }
            try {
                SemanticAnalyzer().analyze(program);
            } catch (const std::exception& error) {
                const std::string message = error.what();
                if (message.rfind("NEXUS found ", 0) == 0 &&
                    message.find(" semantic error") != std::string::npos) {
                    std::istringstream errors(message);
                    std::string line;
                    std::getline(errors, line);
                    while (std::getline(errors, line)) {
                        if (line.rfind("  - ", 0) != 0)
                            continue;
                        const std::runtime_error one(line.substr(4));
                        add(one, "NX2001");
                    }
                    if (diagnostics.empty())
                        add(error, "NX2001");
                } else {
                    add(error, "NX2001");
                }
            }
            applyTokenRanges();
            return diagnostics;
        }

        } // namespace lsp
    } // namespace driver
} // namespace nexus
