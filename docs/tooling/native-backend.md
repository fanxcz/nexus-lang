# Builtin native backend

NEXUS ships a builtin backend that translates supported programs straight
into runnable machine code. No LLVM, Clang, assembler, linker, SDK, or other
development tool is involved at any point, so these programs compile on a
bare machine with only the `nexus` binary next to its standard library.

```sh
nexus build app.nx -o app --backend native
nexus run app.nx --backend native
```

With the default `--backend auto`, NEXUS uses the builtin backend whenever
the program fits the subset below and otherwise falls back to the LLVM
backend (which needs a C++ compiler: `clang++` on `PATH`, `NEXUS_CXX`, or a
bundled wrapper — see `nexus doctor`).

## Targets

| Target | Output | Status |
|---|---|---|
| `x86_64-windows` | PE32+ console `.exe` (imports resolved by the OS loader) | supported |
| `x86_64-linux` | ELF64 executable (direct syscalls only) | supported |
| `native` | host default on x86-64 Windows/Linux | supported |
| `aarch64-*`, `macos`, `wasm32-wasi`, `android` | — | LLVM backend only (Mach-O/AArch64 emitters are future work) |

`nexus targets` lists the LLVM targets and the builtin coverage.
`--backend native` with an unsupported target fails with an explicit
`has no emitter for target` error instead of producing a broken binary.

## Supported subset (v0)

- Types: `i64`, `bool`, `string`, `void`.
- Functions with `i64`/`bool`/`string` parameters and results, including
  recursion; `fn main()` (returning nothing or `i64` exit code) is required.
- Statements: `let`/`mut`, assignment (`=`, `+=`, …), `if`/`else`,
  `while`, desugared `for i in a..b`, `break`, `continue`, `return`.
- Expressions: integer `+ - * / %`, comparisons, boolean `&& || !`,
  unary minus, string concatenation (`+`), string `==`/`!=`, calls.
- I/O: `print(i64 | bool | string)` (trailing newline, like the runtime),
  `input(prompt)` (one stdin line, no trailing newline), `str_i64(i64)`.
- Memory: strings live in a 16 MiB bump arena reserved at startup and
  released at process exit. Exhausting it ends the program with a clean
  `out of string memory` error instead of corrupting memory. There is no
  per-allocation heap overhead and no leak across runs.

Everything else (`f64`, arrays, structs, enums, `match`, pointers,
file/network/graphics/audio/UI/ECS builtins, `extern`) is rejected with a
precise `outside the builtin native v0 subset` reason naming the construct
and function. With `--backend auto` that reason appears only when the LLVM
fallback also cannot proceed; `--backend native` surfaces it directly.

## Verification

- `tests/native_programs/*.nx` are built with `--backend native`, executed,
  and byte-compared against expected output (`ctest -R nexus_native_smoke`).
- The same programs built with `--backend llvm` must produce identical
  output (checked for the core battery during development).
- Emitted images are validated with LLVM `readobj`/`objdump`, and every
  machine-code emitter change is re-checked by disassembly.
- The Windows CI job builds and runs the battery with compilers removed
  from `PATH`; the Linux job additionally executes the ELF output.
