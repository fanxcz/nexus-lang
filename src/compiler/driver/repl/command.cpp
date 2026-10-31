#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {



        int commandRepl() {
            std::cout << "NEXUS " << kVersion << " interactive shell\n"
                      << "Enter an expression to evaluate it. Use let x = 10 to keep a value.\n"
                      << "Commands: :help, :vars, :history, :history-clear, :clear, :quit\n\n";

            const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
            const fs::path tempDir =
                fs::temp_directory_path() / ("nexus-repl-" + std::to_string(tick));
            fs::create_directories(tempDir);

            struct TempDirectoryCleanup {
                fs::path path;
                ~TempDirectoryCleanup() {
                    std::error_code ec;
                    fs::remove_all(path, ec);
                }
            } cleanup{tempDir};

            const fs::path sourcePath = tempDir / "repl.nx";
#ifdef _WIN32
            const fs::path outputPath = tempDir / "repl.exe";
#else
            const fs::path outputPath = tempDir / "repl";
#endif
            std::vector<std::string> declarations;
            std::vector<std::string> statements;
            std::vector<std::string> variableNames;
            const fs::path persistentHistoryPath = replHistoryPath();
            std::vector<std::string> history = loadReplHistory(persistentHistoryPath);
            bool historySaveWarningShown = false;
            const auto persistHistory = [&]() {
                if (!saveReplHistory(persistentHistoryPath, history) && !historySaveWarningShown) {
                    std::cerr << "nexus: warning: could not save REPL history to "
                              << persistentHistoryPath << '\n';
                    historySaveWarningShown = true;
                }
            };

            using ReplValue = std::variant<long long, double, bool, std::string>;
            struct ReplBinding {
                ReplValue value;
                bool mutableValue = false;
            };
            std::unordered_map<std::string, ReplBinding> variables;
            bool fastPathEnabled = true;

            enum class ReplMode { None, Expression, Statement };
            enum class FastLineResult { Handled, Unsupported, Error };

            const auto evalExpression = [&](const auto& self,
                                            const Expr* expression,
                                            ReplValue& result,
                                            std::string& error) -> bool {
                if (!expression)
                    return false;
                if (const auto* literal = dynamic_cast<const IntExpr*>(expression)) {
                    if (literal->literalType.kind != TypeKind::I64)
                        return false;
                    if (literal->negative) {
                        if (literal->value != (std::uint64_t{1} << 63))
                            return false;
                        result = std::numeric_limits<long long>::min();
                        return true;
                    }
                    if (literal->value > static_cast<std::uint64_t>(
                                             std::numeric_limits<long long>::max()))
                        return false;
                    result = static_cast<long long>(literal->value);
                    return true;
                }
                if (const auto* literal = dynamic_cast<const FloatExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* literal = dynamic_cast<const BoolExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* literal = dynamic_cast<const StringExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* variable = dynamic_cast<const VarExpr*>(expression)) {
                    const auto found = variables.find(variable->name);
                    if (found == variables.end())
                        return false;
                    result = found->second.value;
                    return true;
                }
                if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
                    ReplValue operand;
                    if (!self(self, unary->rhs.get(), operand, error))
                        return false;
                    if (unary->op == TokenKind::Bang) {
                        if (const auto* value = std::get_if<bool>(&operand)) {
                            result = !*value;
                            return true;
                        }
                    } else if (unary->op == TokenKind::Minus) {
                        if (const auto* value = std::get_if<long long>(&operand)) {
                            if (*value == std::numeric_limits<long long>::min())
                                return false;
                            result = -*value;
                            return true;
                        }
                        if (const auto* value = std::get_if<double>(&operand)) {
                            result = -*value;
                            return true;
                        }
                    } else if (unary->op == TokenKind::Plus) {
                        if (std::holds_alternative<long long>(operand) ||
                            std::holds_alternative<double>(operand)) {
                            result = operand;
                            return true;
                        }
                    }
                    return false;
                }
                if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
                    ReplValue lhs;
                    ReplValue rhs;
                    if (!self(self, binary->lhs.get(), lhs, error) ||
                        !self(self, binary->rhs.get(), rhs, error))
                        return false;

                    if (binary->op == TokenKind::AndAnd || binary->op == TokenKind::OrOr) {
                        const auto* left = std::get_if<bool>(&lhs);
                        const auto* right = std::get_if<bool>(&rhs);
                        if (!left || !right)
                            return false;
                        result =
                            binary->op == TokenKind::AndAnd ? (*left && *right) : (*left || *right);
                        return true;
                    }

                    const bool lhsInteger = std::holds_alternative<long long>(lhs);
                    const bool rhsInteger = std::holds_alternative<long long>(rhs);
                    const bool lhsNumber = lhsInteger || std::holds_alternative<double>(lhs);
                    const bool rhsNumber = rhsInteger || std::holds_alternative<double>(rhs);

                    if (binary->op == TokenKind::Plus && std::holds_alternative<std::string>(lhs) &&
                        std::holds_alternative<std::string>(rhs)) {
                        result = std::get<std::string>(lhs) + std::get<std::string>(rhs);
                        return true;
                    }

                    if (lhsNumber && rhsNumber) {
                        if (binary->op == TokenKind::EqualEqual ||
                            binary->op == TokenKind::BangEqual || binary->op == TokenKind::Less ||
                            binary->op == TokenKind::LessEqual ||
                            binary->op == TokenKind::Greater ||
                            binary->op == TokenKind::GreaterEqual) {
                            bool comparison = false;
                            if (lhsInteger && rhsInteger) {
                                const auto left = std::get<long long>(lhs);
                                const auto right = std::get<long long>(rhs);
                                switch (binary->op) {
                                case TokenKind::EqualEqual:
                                    comparison = left == right;
                                    break;
                                case TokenKind::BangEqual:
                                    comparison = left != right;
                                    break;
                                case TokenKind::Less:
                                    comparison = left < right;
                                    break;
                                case TokenKind::LessEqual:
                                    comparison = left <= right;
                                    break;
                                case TokenKind::Greater:
                                    comparison = left > right;
                                    break;
                                case TokenKind::GreaterEqual:
                                    comparison = left >= right;
                                    break;
                                default:
                                    return false;
                                }
                            } else {
                                const double left =
                                    lhsInteger ? static_cast<double>(std::get<long long>(lhs))
                                               : std::get<double>(lhs);
                                const double right =
                                    rhsInteger ? static_cast<double>(std::get<long long>(rhs))
                                               : std::get<double>(rhs);
                                switch (binary->op) {
                                case TokenKind::EqualEqual:
                                    comparison = left == right;
                                    break;
                                case TokenKind::BangEqual:
                                    comparison = left != right;
                                    break;
                                case TokenKind::Less:
                                    comparison = left < right;
                                    break;
                                case TokenKind::LessEqual:
                                    comparison = left <= right;
                                    break;
                                case TokenKind::Greater:
                                    comparison = left > right;
                                    break;
                                case TokenKind::GreaterEqual:
                                    comparison = left >= right;
                                    break;
                                default:
                                    return false;
                                }
                            }
                            result = comparison;
                            return true;
                        }

                        if (lhsInteger && rhsInteger) {
                            const long long left = std::get<long long>(lhs);
                            const long long right = std::get<long long>(rhs);
                            if (binary->op == TokenKind::Slash ||
                                binary->op == TokenKind::Percent) {
                                if (right == 0) {
                                    error = "division by zero";
                                    return false;
                                }
                                if (left == std::numeric_limits<long long>::min() && right == -1)
                                    return false;
                                result = binary->op == TokenKind::Slash ? ReplValue(left / right)
                                                                        : ReplValue(left % right);
                                return true;
                            }
                            long long calculated = 0;
                            const auto minimum = std::numeric_limits<long long>::min();
                            const auto maximum = std::numeric_limits<long long>::max();
                            if (binary->op == TokenKind::Plus) {
                                if ((right > 0 && left > maximum - right) ||
                                    (right < 0 && left < minimum - right))
                                    return false;
                                calculated = left + right;
                            } else if (binary->op == TokenKind::Minus) {
                                if ((right < 0 && left > maximum + right) ||
                                    (right > 0 && left < minimum + right))
                                    return false;
                                calculated = left - right;
                            } else if (binary->op == TokenKind::Star) {
                                if (left != 0 && right != 0) {
                                    if ((left == -1 && right == minimum) ||
                                        (right == -1 && left == minimum))
                                        return false;
                                    if (left > 0) {
                                        if ((right > 0 && left > maximum / right) ||
                                            (right < 0 && right < minimum / left))
                                            return false;
                                    } else {
                                        if ((right > 0 && left < minimum / right) ||
                                            (right < 0 && left < maximum / right))
                                            return false;
                                    }
                                }
                                calculated = left * right;
                            } else {
                                return false;
                            }
                            result = calculated;
                            return true;
                        }

                        const double left = lhsInteger
                                                ? static_cast<double>(std::get<long long>(lhs))
                                                : std::get<double>(lhs);
                        const double right = rhsInteger
                                                 ? static_cast<double>(std::get<long long>(rhs))
                                                 : std::get<double>(rhs);
                        if (binary->op == TokenKind::Slash || binary->op == TokenKind::Percent) {
                            if (right == 0.0) {
                                error = "division by zero";
                                return false;
                            }
                            if (binary->op == TokenKind::Percent)
                                result = std::fmod(left, right);
                            else
                                result = left / right;
                            return true;
                        }
                        if (binary->op == TokenKind::Plus)
                            result = left + right;
                        else if (binary->op == TokenKind::Minus)
                            result = left - right;
                        else if (binary->op == TokenKind::Star)
                            result = left * right;
                        else
                            return false;
                        return true;
                    }

                    if (binary->op == TokenKind::EqualEqual || binary->op == TokenKind::BangEqual) {
                        if (lhs.index() != rhs.index())
                            return false;
                        const bool equal = lhs == rhs;
                        result = binary->op == TokenKind::EqualEqual ? equal : !equal;
                        return true;
                    }
                }
                return false;
            };

            const auto tryFastLine =
                [&](const std::string& line, bool stored, bool expression) -> FastLineResult {
                try {
                    const std::string wrapper = "fn __nexus_repl__() {\n" + line + "\n}\n";
                    Program program = Parser(Lexer(wrapper).tokenize()).parseProgram();
                    if (program.functions.size() != 1 || !program.functions.front().body ||
                        program.functions.front().body->statements.size() != 1)
                        return FastLineResult::Unsupported;

                    auto& statement = program.functions.front().body->statements.front();
                    if (stored) {
                        if (auto* declaration = dynamic_cast<LetStmt*>(statement.get())) {
                            if (!declaration->init ||
                                variables.find(declaration->name) != variables.end())
                                return FastLineResult::Unsupported;
                            ReplValue value;
                            std::string error;
                            if (!evalExpression(
                                    evalExpression, declaration->init.get(), value, error)) {
                                if (!error.empty()) {
                                    std::cerr << "nexus: " << error << '\n';
                                    return FastLineResult::Error;
                                }
                                return FastLineResult::Unsupported;
                            }
                            const bool compatible = declaration->type.kind == TypeKind::Unknown ||
                                                    (declaration->type.kind == TypeKind::I64 &&
                                                     std::holds_alternative<long long>(value)) ||
                                                    (declaration->type.kind == TypeKind::F64 &&
                                                     std::holds_alternative<double>(value)) ||
                                                    (declaration->type.kind == TypeKind::Bool &&
                                                     std::holds_alternative<bool>(value)) ||
                                                    (declaration->type.kind == TypeKind::String &&
                                                     std::holds_alternative<std::string>(value));
                            if (!compatible)
                                return FastLineResult::Unsupported;
                            variables.emplace(declaration->name,
                                              ReplBinding{std::move(value), declaration->mut});
                            return FastLineResult::Handled;
                        }
                        if (auto* assignment = dynamic_cast<AssignStmt*>(statement.get())) {
                            const auto found = variables.find(assignment->name);
                            if (found == variables.end() || !found->second.mutableValue)
                                return FastLineResult::Unsupported;
                            ReplValue value;
                            std::string error;
                            if (!evalExpression(
                                    evalExpression, assignment->value.get(), value, error)) {
                                if (!error.empty()) {
                                    std::cerr << "nexus: " << error << '\n';
                                    return FastLineResult::Error;
                                }
                                return FastLineResult::Unsupported;
                            }
                            if (found->second.value.index() != value.index())
                                return FastLineResult::Unsupported;
                            found->second.value = std::move(value);
                            return FastLineResult::Handled;
                        }
                        return FastLineResult::Unsupported;
                    }
                    if (expression) {
                        const auto* expressionStatement =
                            dynamic_cast<const ExprStmt*>(statement.get());
                        if (!expressionStatement || !expressionStatement->expr)
                            return FastLineResult::Unsupported;
                        ReplValue value;
                        std::string error;
                        if (!evalExpression(
                                evalExpression, expressionStatement->expr.get(), value, error)) {
                            if (!error.empty()) {
                                std::cerr << "nexus: " << error << '\n';
                                return FastLineResult::Error;
                            }
                            return FastLineResult::Unsupported;
                        }
                        if (const auto* integer = std::get_if<long long>(&value)) {
                            std::cout << *integer << '\n';
                        } else if (const auto* floating = std::get_if<double>(&value)) {
                            std::cout << std::setprecision(15) << *floating << '\n';
                        } else if (const auto* boolean = std::get_if<bool>(&value)) {
                            std::cout << (*boolean ? "true" : "false") << '\n';
                        } else {
                            std::cout << std::get<std::string>(value) << '\n';
                        }
                        return FastLineResult::Handled;
                    }
                } catch (const std::exception&) {
                    return FastLineResult::Unsupported;
                }
                return FastLineResult::Unsupported;
            };

            const auto execute = [&](const std::vector<std::string>& sourceDeclarations,
                                     const std::vector<std::string>& sourceStatements,
                                     const std::string& currentLine,
                                     ReplMode mode) -> bool {
                std::ofstream source(sourcePath, std::ios::binary | std::ios::trunc);
                if (!source) {
                    std::cerr << "nexus: cannot write temporary REPL source\n";
                    return false;
                }

                for (const auto& declaration : sourceDeclarations) {
                    source << declaration << '\n';
                }
                source << "\nfn main() {\n";
                for (const auto& statement : sourceStatements) {
                    replWriteIndented(source, statement);
                }
                if (mode == ReplMode::Expression) {
                    source << "    print(" << currentLine << ")\n";
                } else if (mode == ReplMode::Statement) {
                    replWriteIndented(source, currentLine);
                }
                source << "}\n";
                source.close();

                std::error_code ec;
                fs::remove(outputPath, ec);
                fs::path llvmIrPath = outputPath;
                llvmIrPath += ".ll";
                fs::remove(llvmIrPath, ec);

                try {
                    const int buildResult =
                        compileFile(sourcePath, outputPath, "native", false, 0, "auto", true);
                    if (buildResult != 0)
                        return false;

                    const int runResult = launchExecutable(outputPath, tempDir);
                    if (runResult != 0) {
                        std::cerr << "nexus: expression exited with status " << runResult << '\n';
                        return false;
                    }
                    return true;
                } catch (const std::exception& error) {
                    std::cerr << formatDiagnostic(error) << '\n';
                    return false;
                }
            };

            std::string pending;
            std::string inputLine;
            const auto buildReplCompletionCandidates = [&]() {
                std::vector<std::string> candidates = {
                    "fn",          "let",        "const",       "mut",          "if",
                    "else",        "while",       "for",         "match",        "return",
                    "struct",      "enum",        "true",        "false",        "print",
                    "input",       "input_i64",   "random_i64",  "str_i64",      "file_read",
                    "file_write",  "json_valid",  "json_get",    "json_type",    "json_array_len",
                    "json_array_get", "json_is_valid", "json_field", "json_value_type",
                    "json_array_length", "json_array_item",
                    "file_exists", "env",         "sleep",       "gfx_window",   "gfx_clear",
                    "gfx_present"};
                candidates.insert(candidates.end(), variableNames.begin(), variableNames.end());
                for (const auto& declaration : declarations) {
                    try {
                        const auto tokens = Lexer(declaration).tokenize();
                        if (tokens.size() < 2 || tokens[1].kind != TokenKind::Identifier)
                            continue;
                        if (tokens[0].kind == TokenKind::KwFn ||
                            tokens[0].kind == TokenKind::KwStruct ||
                            tokens[0].kind == TokenKind::KwEnum) {
                            candidates.push_back(tokens[1].text);
                        }
                    } catch (const std::exception&) {
                        // Invalid/incomplete declarations are already diagnosed when entered.
                    }
                }
                std::sort(candidates.begin(), candidates.end());
                candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
                return candidates;
            };
            for (;;) {
                const std::string prompt = pending.empty() ? "nexus> " : "   ... ";
                std::cout << prompt << std::flush;
                const auto completionCandidates = buildReplCompletionCandidates();
                const auto inputResult =
                    readReplInputLine(prompt, history, completionCandidates, inputLine);
                if (inputResult == ReplInputResult::EndOfInput) {
                    if (!pending.empty()) {
                        std::cerr << "nexus: incomplete multi-line input discarded\n";
                    }
                    break;
                }
                if (inputResult == ReplInputResult::Interrupted) {
                    pending.clear();
                    continue;
                }

                if (pending.empty()) {
                    inputLine = trim(inputLine);
                    if (inputLine.empty())
                        continue;

                    if (inputLine == ":quit" || inputLine == ":q" || inputLine == "quit" ||
                        inputLine == "quit()" || inputLine == "exit" || inputLine == "exit()") {
                        break;
                    }
                    if (inputLine == ":help") {
                        std::cout << "Expressions: 2 + 2, (8 + 4) / 3, 2.5 * 4\n"
                                  << "Bindings:    let answer = 42\n"
                                  << "Assignment:  answer = answer + 1\n"
                                  << "Functions:   fn double(x: i64) -> i64 { return x * 2 }\n"
                                  << "State is rebuilt from saved declarations and statements for "
                                     "each entry.\n"
                                  << ":vars lists bindings; :history shows this session's input; "
                                     ":history-clear clears it.\n"
                                  << ":clear resets code and bindings; :quit exits.\n";
                        continue;
                    }
                    if (inputLine == ":clear") {
                        declarations.clear();
                        statements.clear();
                        variableNames.clear();
                        variables.clear();
                        fastPathEnabled = true;
                        std::cout << "session cleared\n";
                        continue;
                    }
                    if (inputLine == ":vars") {
                        if (variableNames.empty()) {
                            std::cout << "no saved bindings\n";
                        } else {
                            for (const auto& name : variableNames)
                                std::cout << name << '\n';
                        }
                        continue;
                    }
                    if (inputLine == ":history") {
                        if (history.empty()) {
                            std::cout << "history is empty\n";
                        } else {
                            for (std::size_t i = 0; i < history.size(); ++i)
                                std::cout << (i + 1) << "  " << history[i] << '\n';
                        }
                        continue;
                    }
                    if (inputLine == ":history-clear") {
                        history.clear();
                        persistHistory();
                        std::cout << "history cleared\n";
                        continue;
                    }

                    if ((replIsDeclaration(inputLine) || replIsBlockStatement(inputLine)) &&
                        replBraceBalance(inputLine) > 0) {
                        pending = inputLine;
                        continue;
                    }
                } else {
                    pending += '\n';
                    pending += inputLine;
                    if (replBraceBalance(pending) > 0)
                        continue;
                    inputLine = trim(pending);
                    pending.clear();
                }

                history.push_back(inputLine);
                if (history.size() > kReplHistoryLimit) {
                    const auto excess = static_cast<std::vector<std::string>::difference_type>(
                        history.size() - kReplHistoryLimit);
                    history.erase(history.begin(), history.begin() + excess);
                }
                persistHistory();

                if (replIsDeclaration(inputLine)) {
                    auto candidate = declarations;
                    candidate.push_back(inputLine);
                    if (execute(candidate, statements, {}, ReplMode::None)) {
                        declarations = std::move(candidate);
                    }
                    continue;
                }

                // Assignment to a name that does not exist yet becomes an
                // implicit `let mut` binding, so `x = 4` then `x` works.
                {
                    const std::string implicit = replImplicitLet(inputLine);
                    if (!implicit.empty()) {
                        const std::string boundName = replBindingName(implicit);
                        if (std::find(variableNames.begin(), variableNames.end(), boundName) ==
                            variableNames.end()) {
                            inputLine = implicit;
                        }
                    }
                }

                if (replIsStoredStatement(inputLine)) {
                    const auto fastResult = fastPathEnabled ? tryFastLine(inputLine, true, false)
                                                            : FastLineResult::Unsupported;
                    if (fastResult == FastLineResult::Handled) {
                        statements.push_back(inputLine);
                        const std::string name = replBindingName(inputLine);
                        if (!name.empty() &&
                            std::find(variableNames.begin(), variableNames.end(), name) ==
                                variableNames.end()) {
                            variableNames.push_back(name);
                        }
                        continue;
                    }
                    if (fastResult == FastLineResult::Error)
                        continue;

                    auto candidate = statements;
                    candidate.push_back(inputLine);
                    if (execute(declarations, candidate, {}, ReplMode::None)) {
                        statements = std::move(candidate);
                        fastPathEnabled = false;
                        const std::string name = replBindingName(inputLine);
                        if (!name.empty() &&
                            std::find(variableNames.begin(), variableNames.end(), name) ==
                                variableNames.end()) {
                            variableNames.push_back(name);
                        }
                    }
                    continue;
                }

                if (replIsDirectStatement(inputLine) ||
                    replIsVoidFunctionCall(inputLine, declarations)) {
                    const bool isControlBlock = replIsBlockStatement(inputLine);
                    const bool replaySafeBlock = isControlBlock && replIsReplaySafeBlock(inputLine);
                    if (execute(declarations, statements, inputLine, ReplMode::Statement) &&
                        isControlBlock) {
                        fastPathEnabled = false;
                        if (replaySafeBlock) {
                            // Pure state-changing blocks must be replayed after
                            // declarations/assignments so later entries see the same state.
                            statements.push_back(inputLine);
                        } else {
                            std::cerr << "nexus: note: this control-flow block contains calls or "
                                         "unsupported operations, so it is not replayed in later "
                                         "entries; "
                                         "variable changes made inside it will not persist.\n";
                        }
                    }
                    continue;
                }

                if (!inputLine.empty() && inputLine.back() == ';') {
                    inputLine.pop_back();
                    inputLine = trim(inputLine);
                }
                if (inputLine.empty())
                    continue;
                if (fastPathEnabled) {
                    const auto fastResult = tryFastLine(inputLine, false, true);
                    if (fastResult == FastLineResult::Handled ||
                        fastResult == FastLineResult::Error) {
                        continue;
                    }
                }
                execute(declarations, statements, inputLine, ReplMode::Expression);
            }

            std::cout << '\n';
            return 0;
        }

    } // namespace driver
} // namespace nexus
