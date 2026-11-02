#include "lexer/lexer.hpp"

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
        (void)nexus::Lexer(std::move(source)).tokenize();
    } catch (const std::exception&) {
        // Invalid byte sequences and malformed tokens are expected diagnostics.
    } catch (...) {
        // Do not allow a C++ exception to cross the libFuzzer C ABI boundary.
    }
    return 0;
}
