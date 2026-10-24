#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace nexus::native {

    // Position-independent x86-64 machine code plus the data it references.
    // The object writer assigns final addresses; every 32-bit fixup field is
    // patched as:  symbolAddress - (codeBase + at + 4) + addend
    struct CodeImage {
        std::vector<std::uint8_t> bytes;
        std::map<std::string, std::size_t> labels;
        struct Fixup {
            std::size_t at = 0;
            std::string symbol;
            std::int32_t addend = 0;
        };
        std::vector<Fixup> fixups;
        // Data symbols "str<N>" resolve to NUL-free byte strings (may contain '\n').
        std::vector<std::string> strings;
        // PE import symbols "__iat_<name>" resolve to IAT slots (Windows only).
        std::vector<std::string> imports;
    };

    // Emit a runnable Windows x86-64 console executable (PE32+, no relocations,
    // imports resolved by the OS loader). No external tools are used.
    std::vector<std::uint8_t> writeWindowsPE(const CodeImage& image);

    // Emit a runnable Linux x86-64 executable (ELF64, direct syscalls only).
    // No external tools are used.
    std::vector<std::uint8_t> writeLinuxELF(const CodeImage& image);

} // namespace nexus::native
