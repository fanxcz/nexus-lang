#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::string source;
    if (size != 0) {
        source.assign(reinterpret_cast<const char*>(data), size);
    }

    try {
        auto tokens = nexus::Lexer(std::move(source)).tokenize();
        (void)nexus::Parser(std::move(tokens), "<fuzz-input>").parseProgram();
    } catch (const std::exception&) {
        // Invalid tokens and programs should be rejected with a diagnostic.
    } catch (...) {
        // Keep the C ABI exception-safe; sanitizer findings still fail the target.
    }
    return 0;
}
