#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    // A fixed seed makes failures reproducible on Windows, Linux and macOS.
    constexpr std::uint64_t seed = 0x4e455855535f4655ULL;
    constexpr std::size_t iterations = 5000;
    constexpr std::size_t maxLength = 256;
    const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
                                 "{}()[],:;.+-*/%!=&|<>#\"' \t\r\n";
    std::mt19937_64 random(seed);

    for (std::size_t caseIndex = 0; caseIndex < iterations; ++caseIndex) {
        const auto length =
            static_cast<std::size_t>(random() % static_cast<std::uint64_t>(maxLength + 1));
        std::string source;
        source.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            const auto character =
                static_cast<std::size_t>(random() % static_cast<std::uint64_t>(alphabet.size()));
            source.push_back(alphabet[character]);
        }

        try {
            auto tokens = nexus::Lexer(source).tokenize();
            try {
                (void)nexus::Parser(std::move(tokens)).parseProgram();
            } catch (const std::exception&) {
                // Invalid generated programs are expected to be rejected.
            }
        } catch (const std::exception&) {
            // Lexical errors are valid outcomes for malformed input.
        }
    }

    const auto rejects = [](const std::string& source) {
        try {
            auto tokens = nexus::Lexer(source).tokenize();
            (void)nexus::Parser(std::move(tokens)).parseProgram();
            return false;
        } catch (const std::exception&) {
            return true;
        }
    };

    // Numeric conversion errors must remain ordinary, location-aware parser
    // diagnostics rather than escaping as an unclassified standard exception.
    // Scientific-notation literals must be tokenized as one floating-point token.
    const std::vector<std::string> validScientific = {"1e3", "2E+4", "1.25e-3"};
    for (const auto& literal : validScientific) {
        try {
            const auto tokens = nexus::Lexer(literal).tokenize();
            if (tokens.size() < 2 || tokens[0].kind != nexus::TokenKind::Float ||
                tokens[0].text != literal || tokens[1].kind != nexus::TokenKind::End) {
                std::cerr << "scientific-notation tokenization failed for: " << literal << "\n";
                return 10;
            }
        } catch (const std::exception& error) {
            std::cerr << "valid scientific-notation literal rejected: " << literal
                      << ": " << error.what() << "\n";
            return 11;
        }
    }
    for (const std::string literal : {"1e", "1E+", "5.2e-"}) {
        try {
            (void)nexus::Lexer(literal).tokenize();
            std::cerr << "malformed exponent was accepted: " << literal << "\n";
            return 12;
        } catch (const std::exception&) {
            // Malformed literals must be rejected by the lexer.
        }
    }
    if (!rejects("fn main(){ print(1e309) }")) {
        std::cerr << "out-of-range floating-point literal was accepted\n";
        return 13;
    }

    // Numeric separators are allowed only between digits and are removed
    // before conversion to the compiler's integer/floating-point AST values.
    for (const std::string literal : {"1_000", "12_345_678", "1_000.25", "1.2_5e1_0"}) {
        try {
            const auto tokens = nexus::Lexer(literal).tokenize();
            if (tokens.size() < 2 || tokens[0].text != literal ||
                tokens[1].kind != nexus::TokenKind::End) {
                std::cerr << "numeric-separator tokenization failed for: " << literal << "\n";
                return 14;
            }
            const auto expectedKind = literal.find_first_of(".eE") == std::string::npos
                                          ? nexus::TokenKind::Integer
                                          : nexus::TokenKind::Float;
            if (tokens[0].kind != expectedKind) {
                std::cerr << "numeric-separator token kind mismatch for: " << literal << "\n";
                return 15;
            }
        } catch (const std::exception& error) {
            std::cerr << "valid numeric literal rejected: " << literal << ": "
                      << error.what() << "\n";
            return 16;
        }
    }
    for (const std::string literal : {"1_", "1__0", "1_.0", "1e_3", "1.2e3_"}) {
        try {
            (void)nexus::Lexer(literal).tokenize();
            std::cerr << "malformed numeric separator was accepted: " << literal << "\n";
            return 17;
        } catch (const std::exception&) {
            // Separators at invalid positions must be rejected by the lexer.
        }
    }
    // Numeric separators must also normalize in parser-only contexts such as
    // fixed-array lengths, not just expression literals.
    const std::vector<std::string> validNumericPrograms = {
        "fn main(){ print(1_000_000) }",
        "fn main(){ let ratio: f64 = 1.25e-3 }",
        "fn main(value: [i64;1_0]) {}"
    };
    for (const auto& source : validNumericPrograms) {
        try {
            (void)nexus::Parser(nexus::Lexer(source).tokenize()).parseProgram();
        } catch (const std::exception& error) {
            std::cerr << "valid numeric source rejected: " << error.what() << "\n";
            return 18;
        }
    }

    if (!rejects("fn main(){ print(9223372036854775808) }")) {
        std::cerr << "out-of-range integer literal was accepted\n";
        return 1;
    }
    if (!rejects("fn main(value: [i64;999999999999999999999999999999999]) {}")) {
        std::cerr << "out-of-range array length was accepted\n";
        return 2;
    }

    if (rejects("fn main(){ print((((1 + 2)))) }")) {
        std::cerr << "ordinary nested expression was rejected\n";
        return 3;
    }
    std::string deeplyNested = "fn main(){ print(";
    deeplyNested.append(400, '(');
    deeplyNested += "1";
    deeplyNested.append(400, ')');
    deeplyNested += ") }";
    if (!rejects(deeplyNested)) {
        std::cerr << "deeply nested expression was not rejected\n";
        return 4;
    }

    std::string deeplyNestedBlocks = "fn main()";
    for (std::size_t i = 0; i < 300; ++i)
        deeplyNestedBlocks += "{ if true ";
    deeplyNestedBlocks += "{ print(1) }";
    for (std::size_t i = 0; i < 300; ++i)
        deeplyNestedBlocks += " }";
    deeplyNestedBlocks += " }";
    if (!rejects(deeplyNestedBlocks)) {
        std::cerr << "deeply nested blocks were not rejected\n";
        return 5;
    }

    std::cout << "NEXUS frontend fuzz smoke: PASS (" << iterations
              << " deterministic inputs plus parser depth/range regressions, seed " << seed
              << ")\n";
    return 0;
}
