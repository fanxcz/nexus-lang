#include "formatter.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nexus {
    namespace {

        enum class LexemeKind { Word, String, Symbol, Operator, Comment };

        struct Lexeme {
            std::string text;
            LexemeKind kind;
        };

        bool isWordStart(char c) {
            const auto u = static_cast<unsigned char>(c);
            return std::isalnum(u) || c == '_';
        }

        bool isWordContinue(char c) {
            const auto u = static_cast<unsigned char>(c);
            return std::isalnum(u) || c == '_';
        }

        bool isOperator(std::string_view text) {
            return text == "=" || text == "==" || text == "!=" || text == "<" || text == "<=" ||
                   text == ">" || text == ">=" || text == "+" || text == "-" || text == "*" ||
                   text == "/" || text == "%" || text == "+=" || text == "-=" || text == "*=" ||
                   text == "/=" || text == "%=" || text == "&&" || text == "||" || text == "&" ||
                   text == "|" || text == "^" || text == "<<" || text == ">>" || text == "&=" ||
                   text == "|=" || text == "^=" || text == "<<=" || text == ">>=" || text == "!" ||
                   text == "->" || text == "=>";
        }

        bool isUnaryOperator(const std::vector<Lexeme>& tokens, std::size_t index) {
            const auto& op = tokens[index].text;
            if (op != "+" && op != "-" && op != "*" && op != "&" && op != "!")
                return false;
            if (index == 0)
                return true;

            const auto& prev = tokens[index - 1].text;
            return isOperator(prev) || prev == "(" || prev == "[" || prev == "{" || prev == "," ||
                   prev == ";" || prev == ":" || prev == "return" || prev == "let" ||
                   prev == "const" || prev == "mut";
        }

        void consumeBlockComment(const std::string& line,
                                 std::size_t& i,
                                 std::size_t& blockCommentDepth) {
            while (i < line.size() && blockCommentDepth > 0) {
                if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
                    ++blockCommentDepth;
                    i += 2;
                } else if (line[i] == '*' && i + 1 < line.size() && line[i + 1] == '/') {
                    --blockCommentDepth;
                    i += 2;
                } else {
                    ++i;
                }
            }
        }

        std::vector<Lexeme> tokenizeLine(const std::string& line, std::size_t& blockCommentDepth) {
            std::vector<Lexeme> tokens;
            std::size_t i = 0;

            while (i < line.size()) {
                const std::size_t start = i;
                if (std::isspace(static_cast<unsigned char>(line[i]))) {
                    ++i;
                    continue;
                }

                if (blockCommentDepth > 0) {
                    consumeBlockComment(line, i, blockCommentDepth);
                    tokens.push_back({line.substr(start, i - start), LexemeKind::Comment});
                    if (blockCommentDepth > 0)
                        return tokens;
                    continue;
                }

                if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                    tokens.push_back({line.substr(i), LexemeKind::Comment});
                    break;
                }

                if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
                    blockCommentDepth = 1;
                    consumeBlockComment(line, i, blockCommentDepth);
                    tokens.push_back({line.substr(start, i - start), LexemeKind::Comment});
                    if (blockCommentDepth > 0)
                        return tokens;
                    continue;
                }

                if (line[i] == '"') {
                    ++i;
                    bool escaped = false;
                    while (i < line.size()) {
                        const char c = line[i++];
                        if (escaped) {
                            escaped = false;
                        } else if (c == '\\') {
                            escaped = true;
                        } else if (c == '"') {
                            break;
                        }
                    }
                    tokens.push_back({line.substr(start, i - start), LexemeKind::String});
                    continue;
                }

                if (isWordStart(line[i])) {
                    const bool numeric = std::isdigit(static_cast<unsigned char>(line[i]));
                    const bool radixPrefix =
                        numeric && line[i] == '0' && i + 1 < line.size() &&
                        (line[i + 1] == 'x' || line[i + 1] == 'X' || line[i + 1] == 'b' ||
                         line[i + 1] == 'B' || line[i + 1] == 'o' || line[i + 1] == 'O');
                    if (radixPrefix) {
                        i += 2;
                        while (
                            i < line.size() &&
                            (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) {
                            ++i;
                        }
                    } else if (numeric) {
                        while (
                            i < line.size() &&
                            (std::isdigit(static_cast<unsigned char>(line[i])) || line[i] == '_')) {
                            ++i;
                        }

                        // Keep decimal fractions and scientific notation in one
                        // lexeme, including signed exponents such as 1.25E-4.
                        if (i + 1 < line.size() && line[i] == '.' &&
                            std::isdigit(static_cast<unsigned char>(line[i + 1]))) {
                            ++i;
                            while (i < line.size() &&
                                   (std::isdigit(static_cast<unsigned char>(line[i])) ||
                                    line[i] == '_')) {
                                ++i;
                            }
                        }
                        if (i < line.size() && (line[i] == 'e' || line[i] == 'E')) {
                            const auto exponent = i++;
                            if (i < line.size() && (line[i] == '+' || line[i] == '-'))
                                ++i;
                            const auto exponentDigits = i;
                            while (i < line.size() &&
                                   (std::isdigit(static_cast<unsigned char>(line[i])) ||
                                    line[i] == '_')) {
                                ++i;
                            }
                            if (i == exponentDigits)
                                i = exponent;
                        }
                    } else {
                        ++i;
                        while (i < line.size() && isWordContinue(line[i]))
                            ++i;
                    }

                    tokens.push_back({line.substr(start, i - start), LexemeKind::Word});
                    continue;
                }

                static constexpr std::string_view doubleOperators[] = {"==",
                                                                       "!=",
                                                                       "<=",
                                                                       ">=",
                                                                       "+=",
                                                                       "-=",
                                                                       "*=",
                                                                       "/=",
                                                                       "%=",
                                                                       "->",
                                                                       "=>",
                                                                       "&&",
                                                                       "||",
                                                                       "..",
                                                                       "::"};
                bool matched = false;
                for (const auto op : doubleOperators) {
                    if (line.compare(i, op.size(), op) == 0) {
                        i += op.size();
                        const bool arithmetic = op != ".." && op != "::";
                        tokens.push_back({std::string(op),
                                          arithmetic ? LexemeKind::Operator : LexemeKind::Symbol});
                        matched = true;
                        break;
                    }
                }
                if (matched)
                    continue;

                const char c = line[i++];
                const std::string value(1, c);
                const bool op = c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                                c == '=' || c == '<' || c == '>' || c == '!' || c == '&';
                tokens.push_back({value, op ? LexemeKind::Operator : LexemeKind::Symbol});
            }

            return tokens;
        }

        bool isTightBefore(std::string_view value) {
            return value == ")" || value == "]" || value == "," || value == ";" || value == "." ||
                   value == "::" || value == ":";
        }

        bool isTightAfter(std::string_view value) {
            return value == "(" || value == "[" || value == "." || value == "::";
        }

        bool needsSpace(const std::vector<Lexeme>& tokens, std::size_t index) {
            if (index == 0)
                return false;

            const auto& prev = tokens[index - 1].text;
            const auto& current = tokens[index].text;
            if (isTightBefore(current) || isTightAfter(prev))
                return false;

            if (tokens[index].kind == LexemeKind::Comment)
                return prev != "(" && prev != "[";
            if (tokens[index - 1].kind == LexemeKind::Comment)
                return current != ")" && current != "]" && current != "," && current != ";";

            if (current == "{" || prev == "{")
                return prev != "{" || current != "}";
            if (current == "}" || prev == "}") {
                if (current == "}")
                    return prev != "{";
                return prev != "(";
            }

            if (current == "(") {
                return prev == "if" || prev == "while" || prev == "match";
            }

            if (current == ":")
                return false;
            if (prev == ":")
                return true;

            const bool currentUnary = isUnaryOperator(tokens, index);
            const bool prevUnary = isUnaryOperator(tokens, index - 1);
            const bool currentOp = isOperator(current);
            const bool prevOp = isOperator(prev);

            if (currentOp || prevOp) {
                if (currentUnary)
                    return prevOp;
                if (prevUnary)
                    return false;
                return true;
            }

            if (tokens[index - 1].kind == LexemeKind::Word ||
                tokens[index - 1].kind == LexemeKind::String) {
                return tokens[index].kind == LexemeKind::Word ||
                       tokens[index].kind == LexemeKind::String || current == "{" ||
                       current == "else";
            }

            return prev == "," || prev == ";" || prev == "else" || prev == "return" ||
                   prev == "let" || prev == "const" || prev == "mut" || prev == "fn" ||
                   prev == "in";
        }

        std::string joinTokens(const std::vector<Lexeme>& tokens) {
            std::string result;
            for (std::size_t i = 0; i < tokens.size(); ++i) {
                if (needsSpace(tokens, i) && !result.empty() && result.back() != ' ')
                    result.push_back(' ');
                result += tokens[i].text;
            }
            return result;
        }

        std::size_t leadingClosingBraces(const std::vector<Lexeme>& tokens) {
            std::size_t count = 0;
            bool foundCode = false;
            for (const auto& token : tokens) {
                if (token.kind == LexemeKind::Comment)
                    continue;
                if (!foundCode && token.text == "}") {
                    ++count;
                    continue;
                }
                foundCode = true;
                break;
            }
            return count;
        }

        int braceDelta(const std::vector<Lexeme>& tokens) {
            int delta = 0;
            for (const auto& token : tokens) {
                if (token.kind == LexemeKind::Comment || token.kind == LexemeKind::String)
                    continue;
                if (token.text == "{")
                    ++delta;
                else if (token.text == "}")
                    --delta;
            }
            return delta;
        }

    } // namespace

    std::string formatNexusSource(const std::string& source) {
        std::istringstream input(source);
        std::ostringstream output;
        std::string line;
        std::size_t depth = 0;
        std::size_t blockCommentDepth = 0;

        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            const bool blank = line.find_first_not_of(" \t\r\n") == std::string::npos;
            if (blank) {
                output << '\n';
                continue;
            }

            const auto tokens = tokenizeLine(line, blockCommentDepth);
            const std::size_t closers = leadingClosingBraces(tokens);
            const std::size_t indent = closers > depth ? 0 : depth - closers;
            const std::string formatted = joinTokens(tokens);

            for (std::size_t i = 0; i < indent; ++i)
                output << "    ";
            output << formatted << '\n';

            const auto delta = braceDelta(tokens);
            if (delta < 0) {
                const auto closes = static_cast<std::size_t>(-delta);
                depth = closes > depth ? 0 : depth - closes;
            } else {
                depth += static_cast<std::size_t>(delta);
            }
        }

        const std::string formatted = output.str();

        // Keep the formatter semantics-preserving for any source that already
        // parses. Parseability alone is not sufficient: a whitespace regression
        // might still produce a different (but valid) token stream. Lexer token
        // text captures decoded string contents, so escaped strings are protected
        // as well. Comments and whitespace are intentionally not tokens.
        std::vector<Token> originalTokens;
        bool originalParses = false;
        try {
            originalTokens = Lexer(source).tokenize();
            (void)Parser(originalTokens).parseProgram();
            originalParses = true;
        } catch (const std::exception&) {
            // Incomplete/invalid input is still formatted conservatively below.
        }
        if (originalParses) {
            try {
                const auto formattedTokens = Lexer(formatted).tokenize();
                if (originalTokens.size() != formattedTokens.size())
                    return source;
                for (std::size_t i = 0; i < originalTokens.size(); ++i) {
                    if (originalTokens[i].kind != formattedTokens[i].kind ||
                        originalTokens[i].text != formattedTokens[i].text)
                        return source;
                }

                (void)Parser(formattedTokens).parseProgram();
            } catch (const std::exception&) {
                return source;
            }
        }

        return formatted;
    }

} // namespace nexus
