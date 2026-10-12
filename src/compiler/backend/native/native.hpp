#pragma once
#include "ast/ast.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace nexus::native {

// Builtin direct machine-code backend (v0).
//
// Translates the supported NEXUS subset straight into an executable image.
// No LLVM, Clang, assembler, or linker is involved at any point, so programs
// inside the subset compile on a bare machine with only the `nexus` binary,
// its standard library sources, and an OS that can run the output.
//
// Supported v0 subset:
//   types:      i64, bool, string, void
//   functions:  i64/bool/string params, i64/bool/string/void results,
//               recursion allowed
//   statements: let/mut, assignment, if/else, while, for-in ranges,
//               break, continue, return, expression statements
//   expressions: integer arithmetic (+ - * / %), string concatenation (+),
//               comparisons (including string ==/!=), boolean && || !,
//               unary minus, calls, print()
//   output:     print(i64), print(bool), print(string)
//   input:      input(prompt) reads one stdin line, str_i64(i64)
//   memory:     strings live in a 16 MiB bump arena reserved at startup
//               (freed at process exit); exhausting it is a clean error
//
// Everything else (f64, arrays, structs, enums, match, pointers,
// file/network/graphics/audio/UI/ECS builtins, extern functions) is rejected
// with a precise reason so the driver can fall back to the LLVM backend.
enum class NativeTarget { WindowsX64, LinuxX64 };

// True when `target` selects the builtin emitter ("native", "x86_64-windows",
// "x86_64-linux", and matching LLVM triples are accepted).
bool resolveTarget(const std::string& target, NativeTarget& out);

// True when the whole program fits the v0 subset; otherwise false with a
// human-readable `reason` naming the first unsupported construct.
bool supports(const Program& program, std::string& reason);

// Build a runnable executable image. Throws std::runtime_error when the
// program is outside the subset or the target has no emitter yet.
std::vector<std::uint8_t> buildExecutable(const Program& program, NativeTarget target);

} // namespace nexus::native
