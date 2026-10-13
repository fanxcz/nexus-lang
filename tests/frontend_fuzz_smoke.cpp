#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <utility>

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
