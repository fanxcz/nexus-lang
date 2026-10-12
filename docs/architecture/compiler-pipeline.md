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

The semantic pass validates declarations, names, mutability, expression types, structs, enums, function signatures and runtime built-ins. Some advanced language capabilities remain experimental and are documented separately.

### Backend — `src/compiler/backend/codegen/`

`llvm_ir.cpp` lowers the validated program to LLVM IR. The driver invokes the available native Clang toolchain to link the generated IR with `src/runtime/runtime.cpp`.

### Driver and tooling — `src/compiler/driver/`

`driver.cpp` provides the CLI workflows: project creation, checking, building/running programs, standard-module resolution, package commands, formatter/linter entry points, and the LSP transport.

## Source diagnostics

The driver associates parse errors with the source file being processed and renders a path, line, column, the offending source line, and a caret. Diagnostics from stages that do not expose a source position still use the generic `NX0000` error format.

## Important boundaries

- The AST is the shared interface between parser, semantic pass and code generator.
- Runtime implementation lives in `src/runtime/` and is linked into generated programs.
- Standard modules live in `stdlib/`; built-in project starters live in `examples/templates/`.
- Native toolchain availability and target sysroots still affect cross-compilation.
- Generics, trait dispatch, closure/async lowering and a complete JSON AST are not stable implemented features yet; they must not be inferred from the directory layout or future roadmap.
