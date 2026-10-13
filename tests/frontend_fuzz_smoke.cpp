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
    constexpr std::size_t iterations = 1500;
    constexpr std::size_t maxLength = 160;
    const std::string alphabet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
        "{}()[],:;.+-*/%!=&|<>#\"' \t\r\n";
    std::mt19937_64 random(seed);

    for (std::size_t caseIndex = 0; caseIndex < iterations; ++caseIndex) {
        const auto length = static_cast<std::size_t>(
            random() % static_cast<std::uint64_t>(maxLength + 1));
        std::string source;
        source.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            const auto character = static_cast<std::size_t>(
                random() % static_cast<std::uint64_t>(alphabet.size()));
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

    std::cout << "NEXUS frontend fuzz smoke: PASS (" << iterations
              << " deterministic inputs, seed " << seed << ")\n";
    return 0;
}
