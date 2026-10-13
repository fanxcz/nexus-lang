#include "formatter.hpp"

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
    return text == "=" || text == "==" || text == "!=" || text == "<" ||
           text == "<=" || text == ">" || text == ">=" || text == "+" ||
           text == "-" || text == "*" || text == "/" || text == "%" ||
           text == "+=" || text == "-=" || text == "*=" || text == "/=" ||
           text == "%=" || text == "&&" || text == "||" || text == "&" ||
           text == "!" || text == "->" || text == "=>";
}

bool isUnaryOperator(const std::vector<Lexeme>& tokens, std::size_t index) {
    const auto& op = tokens[index].text;
    if (op != "+" && op != "-" && op != "*" && op != "&" && op != "!")
        return false;
    if (index == 0)
        return true;

    const auto& prev = tokens[index - 1].text;
    return isOperator(prev) || prev == "(" || prev == "[" || prev == "{" ||
           prev == "," || prev == ";" || prev == ":" ||
           prev == "return" || prev == "let" || prev == "const" ||
           prev == "mut";
}

std::vector<Lexeme> tokenizeLine(const std::string& line, bool& inBlockComment) {
    std::vector<Lexeme> tokens;
    std::size_t i = 0;

    while (i < line.size()) {
        const std::size_t start = i;
        if (std::isspace(static_cast<unsigned char>(line[i]))) {
            ++i;
            continue;
        }

        if (inBlockComment) {
            const auto close = line.find("*/", i);
            if (close == std::string::npos) {
                tokens.push_back({line.substr(i), LexemeKind::Comment});
                return tokens;
            }
            i = close + 2;
            tokens.push_back({line.substr(start, i - start), LexemeKind::Comment});
            inBlockComment = false;
            continue;
        }

        if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            tokens.push_back({line.substr(i), LexemeKind::Comment});
            break;
        }

        if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
            const auto close = line.find("*/", i + 2);
            if (close == std::string::npos) {
                tokens.push_back({line.substr(i), LexemeKind::Comment});
                inBlockComment = true;
                break;
            }
            i = close + 2;
            tokens.push_back({line.substr(start, i - start), LexemeKind::Comment});
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
            ++i;
            while (i < line.size() && isWordContinue(line[i]))
                ++i;

            // Keep decimal literals as one token so spacing never changes them.
            if (std::isdigit(static_cast<unsigned char>(line[start])) &&
                i + 1 < line.size() && line[i] == '.' &&
                std::isdigit(static_cast<unsigned char>(line[i + 1]))) {
                ++i;
                while (i < line.size() &&
                       std::isdigit(static_cast<unsigned char>(line[i])))
                    ++i;
            }

            tokens.push_back({line.substr(start, i - start), LexemeKind::Word});
            continue;
        }

        static constexpr std::string_view doubleOperators[] = {
            "==", "!=", "<=", ">=", "+=", "-=", "*=", "/=", "%=",
            "->", "=>", "&&", "||", "..", "::"
        };
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
        const bool op = c == '+' || c == '-' || c == '*' || c == '/' ||
                        c == '%' || c == '=' || c == '<' || c == '>' ||
                        c == '!' || c == '&';
        tokens.push_back({value, op ? LexemeKind::Operator : LexemeKind::Symbol});
    }

    return tokens;
}

bool isTightBefore(std::string_view value) {
    return value == ")" || value == "]" || value == "," || value == ";" ||
           value == "." || value == "::" || value == ":";
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
    if (current == "}" || prev == "}")
        return current != "}" && prev != "(";

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
               tokens[index].kind == LexemeKind::String ||
               current == "{" || current == "else";
    }

    return prev == "," || prev == ";" || prev == "else" ||
           prev == "return" || prev == "let" || prev == "const" ||
           prev == "mut" || prev == "fn" || prev == "in";
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
    bool inBlockComment = false;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        const bool blank = line.find_first_not_of(" \t\r\n") == std::string::npos;
        if (blank) {
            output << '\n';
            continue;
        }

        const auto tokens = tokenizeLine(line, inBlockComment);
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

    return output.str();
}

} // namespace nexus
