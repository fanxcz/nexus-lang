# Compiler Pipeline

NEXUS uses a native, multi-stage compiler pipeline. The current implementation is deliberately smaller than a complete LLVM-based production language toolchain; the stages below describe the code that is actually in the repository.

## Pipeline

```text
Source (.nx)
  → Lexer: source text to positioned tokens
  → Parser: tokens to AST
  → Semantic analysis: validate names, bindings and types
  → LLVM IR generation
  → Clang/LLVM native compiler and linker
  → Native executable
```

## Implementation layout

### Lexer — `src/compiler/frontend/lexer/`

`lexer.cpp` tokenizes keywords, identifiers, numeric and string literals, operators, and punctuation. Tokens carry a source position with line, column and offset.

### Parser — `src/compiler/frontend/parser/`

`parser.cpp` uses recursive descent for declarations, statements, types, expressions and precedence. Syntax errors include the current token's line and column.

### AST — `src/compiler/ast/`

`ast.hpp` defines the syntax tree shared by parsing, semantic analysis and code generation.

### Semantic analysis — `src/compiler/semantic/`

The semantic pass validates declarations, names, mutability, expression types, structs, enums, function signatures and runtime built-ins. Built-in signatures live under `src/compiler/builtins/`, grouped by topic (`core`, `strings`, `collections`, `graphics`, `screen`, `ui`, `audio`, `game`, `math`, `data`), with `builtins.hpp` as the lookup facade. Some advanced language capabilities remain experimental and are documented separately.

### Backends — `src/compiler/backend/`

There are two backends that share the same validated AST and semantics.

- **LLVM IR** (`codegen/`): `llvm_ir.cpp` is the public entry point, `llvm_cg.hpp` declares the internal `CG` emitter, and the implementation is split by topic across `llvm_cg_core.cpp` (module/type/struct helpers), `llvm_cg_expr.cpp` (expressions, defer, compound assignment), `llvm_cg_stmt.cpp` (statements), and `llvm_cg_program.cpp` (program layout). The driver invokes the available native Clang toolchain to link the generated IR with the runtime translation unit `src/runtime/runtime.cpp`, which is assembled from the topic files under `src/runtime/`.
- **Builtin native** (`native/`): `native.cpp` exposes the supported-subset check and executable writer, `native_subset.cpp` implements the subset type checker, `x64_emitter.hpp` holds the x86-64 instruction encoder, `native_generator.hpp` declares the code generator, and `native_gen_core.cpp`, `native_gen_stmt.cpp` and `native_gen_runtime.cpp` implement it by topic.

### Driver and tooling — `src/compiler/driver/`

`driver.cpp` orchestrates the CLI workflows. The remaining driver code is grouped by responsibility under `cli/`, `commands/`, `lsp/`, `package/`, `project/`, `repl/`, `support/`, and `toolchain/`, with the shared internal interface in `driver_internal.hpp`. The LSP transport in `lsp/` keeps the protocol loop in `server.cpp` and puts reusable JSON-RPC framing (`protocol.cpp`), UTF-16/line helpers (`text.cpp`), URI conversion (`uri.cpp`), and diagnostic collection (`diagnostics.cpp`) in separate units.

## Source diagnostics

The driver associates parse errors with the source file being processed and renders a path, line, column, the offending source line, and a caret. Diagnostics from stages that do not expose a source position still use the generic `NX0000` error format.

## Important boundaries

- The AST is the shared interface between parser, semantic pass and code generator.
- Runtime implementation lives in `src/runtime/` and is linked into generated programs.
- Standard modules live in `stdlib/`; built-in project starters live in `examples/templates/`.
- Native toolchain availability and target sysroots still affect cross-compilation.
- Generics, trait dispatch, closure/async lowering and a complete JSON AST are not stable implemented features yet; they must not be inferred from the directory layout or future roadmap.
