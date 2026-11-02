# Compiler Architecture

This page describes the compiler at the current source revision. The implementation, tests and CMake source lists are authoritative when a detail changes.

## Pipeline

```text
.nx source
  -> project/import loader (driver)
  -> lexer: source text -> positioned tokens
  -> parser: tokens -> shared AST
  -> semantic analyzer: names, types, mutability, borrows, builtin signatures
  -> backend selection
       -> builtin native emitter -> x86-64 PE32+ (Windows) / ELF64 (Linux)
       -> LLVM IR generator -> clang++-compatible compiler + runtime -> executable
```

Both backends consume the validated AST. The `auto` path prefers the builtin emitter when the whole program fits its subset and otherwise attempts the LLVM path. LLVM compilation requires a compatible C++ compiler; SDL2 is optional at compiler configure time and affects graphics/runtime availability.

## Directory map

| Path | Responsibility |
|---|---|
| `src/cli/` | Process entry point |
| `src/compiler/ast/` | Shared AST and source spans |
| `src/compiler/frontend/lexer/` | Tokenization and literal handling |
| `src/compiler/frontend/parser/` | Recursive-descent declarations, statements, types and expressions |
| `src/compiler/semantic/` | Name/type checking, mutability and conservative borrow/lifetime checks |
| `src/compiler/backend/native/` | Native subset validation, x86-64 emitter, PE/ELF image writing |
| `src/compiler/backend/codegen/` | LLVM IR generation |
| `src/compiler/builtins/` | Runtime builtin signatures, grouped by topic |
| `src/compiler/driver/` | CLI, imports, packages, REPL, LSP and toolchain orchestration |
| `src/runtime/` | Runtime translation unit and topic-based `.inc` implementation |
| `stdlib/` | Importable NEXUS standard-library modules |
| `examples/` | Examples, assets and project templates |
| `tests/` | C++ unit tests, compiler smoke tests and source fixtures |
| `tests/fuzz/` | Opt-in libFuzzer harnesses and seed corpora |
| `tests/differential/` | Native/LLVM output comparison |
| `tests/negative/` | Expected compiler diagnostics |
| `benchmarks/` | Compile-time and runtime measurement harness |
| `docs/` | User, architecture and contributor documentation |
| `.github/workflows/` | CI, release, security and maintenance automation |

## Where to make common changes

- Token behavior and numeric/string literals: `src/compiler/frontend/lexer/`.
- Grammar, precedence and parser diagnostics: `src/compiler/frontend/parser/` and `src/compiler/ast/`.
- Types, ownership/mutability and semantic diagnostics: `src/compiler/semantic/`.
- Builtin native support: `src/compiler/backend/native/`; update the subset documentation and add native regression cases.
- LLVM lowering: `src/compiler/backend/codegen/`; add focused compile/run coverage.
- Builtin declarations: `src/compiler/builtins/`; runtime implementation: matching `src/runtime/<topic>/` files.
- CLI, module loading or toolchain discovery: `src/compiler/driver/`.
- Standard modules: `stdlib/*.nx` and their matching `docs/std/` pages.

The runtime is assembled into a translation unit from topic files under `src/runtime/`. SDL2 is optional during configure: a build can complete without it, but graphics-related runtime features may be unavailable. A compiler built on one host does not imply that the builtin emitter can target that host. See the [backend support matrix](backend-support-matrix.md) and the existing [architecture reference](architecture/compiler-pipeline.md).
