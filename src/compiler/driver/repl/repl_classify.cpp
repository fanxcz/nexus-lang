#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {

        bool replStartsWith(const std::string& value, std::string_view prefix) {
            return value.size() >= prefix.size() &&
                   value.compare(0, prefix.size(), prefix.data(), prefix.size()) == 0;
        }


        int replBraceBalance(const std::string& source) {
            int balance = 0;
            bool inString = false;
            bool escaped = false;
            bool inBlockComment = false;

            for (std::size_t i = 0; i < source.size(); ++i) {
                const char ch = source[i];
                const bool hasNext = i + 1 < source.size();
                const char next = hasNext ? source[i + 1] : '\0';

                if (inBlockComment) {
                    if (ch == '*' && next == '/') {
                        inBlockComment = false;
                        ++i;
                    }
                    continue;
                }
                if (inString) {
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        inString = false;
                    }
                    continue;
                }
                if (ch == '/' && next == '/') {
                    while (i < source.size() && source[i] != '\n')
                        ++i;
                    continue;
                }
                if (ch == '/' && next == '*') {
                    inBlockComment = true;
                    ++i;
                    continue;
                }
                if (ch == '"') {
                    inString = true;
                } else if (ch == '{') {
                    ++balance;
                } else if (ch == '}') {
                    --balance;
                }
            }
            return balance;
        }


        bool replHasAssignment(const std::string& source) {
            bool inString = false;
            bool escaped = false;

            for (std::size_t i = 0; i < source.size(); ++i) {
                const char ch = source[i];

                if (inString) {
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        inString = false;
                    }
                    continue;
                }
                if (ch == '"') {
                    inString = true;
                    continue;
                }
                if (ch == '=' && (i + 1 == source.size() || source[i + 1] != '=') &&
                    (i == 0 || std::string("=!<>").find(source[i - 1]) == std::string::npos)) {
                    return i > 0;
                }
            }
            return false;
        }


        bool replIsDeclaration(const std::string& line) {
            return replStartsWith(line, "fn ") || replStartsWith(line, "struct ") ||
                   replStartsWith(line, "enum ") || replStartsWith(line, "extern ") ||
                   replStartsWith(line, "import ");
        }


        // `x = 4` typed by a user who has not declared `x` yet: rewrite to
        // `let mut x = 4` so the REPL feels like Python instead of erroring.
        // Only a bare identifier target qualifies; `a[i] = v`, `s.f = v`,
        // compound `+=` and shadowed existing bindings are left untouched.
        std::string replImplicitLet(const std::string& source) {
            std::string line = trim(source);
            if (!line.empty() && line.back() == ';') {
                line.pop_back();
                line = trim(line);
            }
            if (line.empty() ||
                !(std::isalpha(static_cast<unsigned char>(line.front())) || line.front() == '_'))
                return {};
            std::size_t nameEnd = 1;
            while (nameEnd < line.size()) {
                const unsigned char ch = static_cast<unsigned char>(line[nameEnd]);
                if (!std::isalnum(ch) && ch != '_')
                    break;
                ++nameEnd;
            }
            const std::string name = line.substr(0, nameEnd);
            std::size_t i = nameEnd;
            while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
                ++i;
            if (i >= line.size() || line[i] != '=')
                return {};
            if (i + 1 >= line.size() || line[i + 1] == '=')
                return {};
            return "let mut " + name + " = " + trim(line.substr(i + 1));
        }


        bool replIsBlockStatement(const std::string& line) {
            return replStartsWith(line, "if ") || replStartsWith(line, "while ") ||
                   replStartsWith(line, "for ") || replStartsWith(line, "match ") ||
                   replStartsWith(line, "unsafe ");
        }


        bool replExpressionIsReplaySafe(const Expr* expression) {
            if (!expression)
                return true;
            if (dynamic_cast<const IntExpr*>(expression) ||
                dynamic_cast<const FloatExpr*>(expression) ||
                dynamic_cast<const BoolExpr*>(expression) ||
                dynamic_cast<const StringExpr*>(expression) ||
                dynamic_cast<const VarExpr*>(expression) ||
                dynamic_cast<const EnumVariantExpr*>(expression))
                return true;
            if (const auto* array = dynamic_cast<const ArrayExpr*>(expression)) {
                for (const auto& element : array->elements)
                    if (!replExpressionIsReplaySafe(element.get()))
                        return false;
                return true;
            }
            if (const auto* index = dynamic_cast<const IndexExpr*>(expression))
                return replExpressionIsReplaySafe(index->object.get()) &&
                       replExpressionIsReplaySafe(index->index.get());
            if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression))
                return replExpressionIsReplaySafe(unary->rhs.get());
            if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression))
                return replExpressionIsReplaySafe(binary->lhs.get()) &&
                       replExpressionIsReplaySafe(binary->rhs.get());
            if (const auto* member = dynamic_cast<const MemberExpr*>(expression))
                return replExpressionIsReplaySafe(member->object.get());
            if (const auto* initializer = dynamic_cast<const StructInitExpr*>(expression)) {
                for (const auto& field : initializer->fields)
                    if (!replExpressionIsReplaySafe(field.second.get()))
                        return false;
                return true;
            }

            // Calls are intentionally excluded: built-ins and user functions can
            // perform I/O or otherwise change external state when replayed.
            return false;
        }


        bool replStatementIsReplaySafe(const Stmt* statement) {
            if (!statement)
                return false;
            if (const auto* block = dynamic_cast<const Block*>(statement)) {
                for (const auto& nested : block->statements)
                    if (!replStatementIsReplaySafe(nested.get()))
                        return false;
                return true;
            }
            if (const auto* declaration = dynamic_cast<const LetStmt*>(statement))
                return replExpressionIsReplaySafe(declaration->init.get());
            if (const auto* assignment = dynamic_cast<const AssignStmt*>(statement))
                return replExpressionIsReplaySafe(assignment->value.get());
            if (const auto* assignment = dynamic_cast<const MemberAssignStmt*>(statement))
                return replExpressionIsReplaySafe(assignment->object.get()) &&
                       replExpressionIsReplaySafe(assignment->value.get());
            if (const auto* assignment = dynamic_cast<const IndexAssignStmt*>(statement))
                return replExpressionIsReplaySafe(assignment->object.get()) &&
                       replExpressionIsReplaySafe(assignment->index.get()) &&
                       replExpressionIsReplaySafe(assignment->value.get());
            if (const auto* expression = dynamic_cast<const ExprStmt*>(statement))
                return replExpressionIsReplaySafe(expression->expr.get());
            if (const auto* branch = dynamic_cast<const IfStmt*>(statement))
                return replExpressionIsReplaySafe(branch->cond.get()) &&
                       replStatementIsReplaySafe(branch->thenBlock.get()) &&
                       (!branch->elseBlock || replStatementIsReplaySafe(branch->elseBlock.get()));
            if (const auto* loop = dynamic_cast<const WhileStmt*>(statement))
                return replExpressionIsReplaySafe(loop->cond.get()) &&
                       replStatementIsReplaySafe(loop->body.get());
            if (const auto* match = dynamic_cast<const MatchStmt*>(statement)) {
                if (!replExpressionIsReplaySafe(match->value.get()))
                    return false;
                for (const auto& arm : match->arms) {
                    if ((!arm.wildcard && !replExpressionIsReplaySafe(arm.pattern.get())) ||
                        !replStatementIsReplaySafe(arm.body.get()))
                        return false;
                }
                return true;
            }
            if (dynamic_cast<const BreakStmt*>(statement) ||
                dynamic_cast<const ContinueStmt*>(statement))
                return true;

            // Return statements and unknown statement kinds can change the control
            // flow of later replayed entries, so do not persist them.
            return false;
        }


        bool replIsReplaySafeBlock(const std::string& line) {
            try {
                const std::string wrapper = "fn __nexus_repl__() {\n" + line + "\n}\n";
                Program program = Parser(Lexer(wrapper).tokenize()).parseProgram();
                if (program.functions.size() != 1 || !program.functions.front().body ||
                    program.functions.front().body->statements.size() != 1)
                    return false;
                return replStatementIsReplaySafe(
                    program.functions.front().body->statements.front().get());
            } catch (const std::exception&) {
                return false;
            }
        }


        bool replIsStoredStatement(const std::string& line) {
            return replStartsWith(line, "let ") || replStartsWith(line, "const ") ||
                   replHasAssignment(line);
        }


        bool replIsDirectStatement(const std::string& line) {
            return replStartsWith(line, "print(") || replStartsWith(line, "print (") ||
                   replIsBlockStatement(line);
        }


        bool replBareFunctionCall(const std::string& source, std::string& functionName) {
            std::string line = trim(source);
            if (!line.empty() && line.back() == ';') {
                line.pop_back();
                line = trim(line);
            }
            if (line.empty() ||
                !(std::isalpha(static_cast<unsigned char>(line.front())) || line.front() == '_'))
                return false;

            std::size_t nameEnd = 1;
            while (nameEnd < line.size()) {
                const unsigned char ch = static_cast<unsigned char>(line[nameEnd]);
                if (!std::isalnum(ch) && ch != '_')
                    break;
                ++nameEnd;
            }
            functionName = line.substr(0, nameEnd);
            std::size_t open = nameEnd;
            while (open < line.size() && std::isspace(static_cast<unsigned char>(line[open])))
                ++open;
            if (open >= line.size() || line[open] != '(')
                return false;

            std::size_t depth = 0;
            bool inString = false;
            bool escaped = false;
            for (std::size_t i = open; i < line.size(); ++i) {
                const char ch = line[i];
                const bool hasNext = i + 1 < line.size();
                const char next = hasNext ? line[i + 1] : '\0';
                if (inString) {
                    if (escaped)
                        escaped = false;
                    else if (ch == '\\')
                        escaped = true;
                    else if (ch == '"')
                        inString = false;
                    continue;
                }
                if (ch == '/' && next == '/')
                    return trim(line.substr(i)).empty();
                if (ch == '/' && next == '*') {
                    const auto close = line.find("*/", i + 2);
                    if (close == std::string::npos)
                        return false;
                    i = close + 1;
                    continue;
                }
                if (ch == '"') {
                    inString = true;
                } else if (ch == '(') {
                    ++depth;
                } else if (ch == ')') {
                    if (depth == 0)
                        return false;
                    --depth;
                    if (depth == 0)
                        return trim(line.substr(i + 1)).empty();
                }
            }
            return false;
        }


        bool replIsVoidFunctionCall(const std::string& line,
                                    const std::vector<std::string>& declarations) {
            std::string calledName;
            if (!replBareFunctionCall(line, calledName))
                return false;

            for (const auto& declaration : declarations) {
                const std::string header = trim(declaration);
                if (!replStartsWith(header, "fn "))
                    continue;
                std::size_t nameStart = 3;
                while (nameStart < header.size() &&
                       std::isspace(static_cast<unsigned char>(header[nameStart])))
                    ++nameStart;
                std::size_t nameEnd = nameStart;
                while (nameEnd < header.size()) {
                    const unsigned char ch = static_cast<unsigned char>(header[nameEnd]);
                    if (!std::isalnum(ch) && ch != '_')
                        break;
                    ++nameEnd;
                }
                if (header.substr(nameStart, nameEnd - nameStart) != calledName)
                    continue;
                const auto bodyStart = header.find('{', nameEnd);
                const auto declarationEnd =
                    bodyStart == std::string::npos ? header.find(';', nameEnd) : bodyStart;
                const std::string signature =
                    header.substr(nameEnd,
                                  declarationEnd == std::string::npos ? std::string::npos
                                                                      : declarationEnd - nameEnd);
                return signature.find("->") == std::string::npos;
            }
            return false;
        }


        std::string replBindingName(const std::string& line) {
            std::string rest;
            if (replStartsWith(line, "let ")) {
                rest = trim(line.substr(4));
                if (replStartsWith(rest, "mut "))
                    rest = trim(rest.substr(4));
            } else if (replStartsWith(line, "const ")) {
                rest = trim(line.substr(6));
            } else {
                return {};
            }

            std::size_t end = 0;
            while (end < rest.size()) {
                const unsigned char ch = static_cast<unsigned char>(rest[end]);
                if (!std::isalnum(ch) && rest[end] != '_')
                    break;
                ++end;
            }
            return rest.substr(0, end);
        }


        void replWriteIndented(std::ostream& output, const std::string& source) {
            std::istringstream input(source);
            std::string line;
            while (std::getline(input, line)) {
                output << "    " << line << '\n';
            }
        }


    } // namespace driver
} // namespace nexus
