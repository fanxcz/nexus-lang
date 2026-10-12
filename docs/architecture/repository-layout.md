# Repository Architecture

NEXUS is organized as a layered native toolchain. The source tree is grouped by responsibility; the portable Windows package has its own runtime layout and is assembled by CI.

## Repository root

The repository has exactly six top-level directories:

```text
nexus/
├── .github/
│   ├── workflows/
│   └── project-tools/
│       ├── installer/
│       └── scripts/
├── docs/
├── examples/
│   ├── assets/
│   └── templates/
├── src/
├── stdlib/
└── tests/
```

Root files such as `CMakeLists.txt`, `README.md`, and `LICENSE` remain at the root. Helper scripts are grouped under `.github/project-tools/`, while starter sources live in `examples/templates/`. Built packages retain the user-facing `project-templates/` directory name for compatibility.

## Source layout

| Path | Responsibility |
|---|---|
| `src/cli/` | Process entry point and CLI bootstrap |
| `src/compiler/ast/` | Shared syntax-tree model |
| `src/compiler/frontend/lexer/` | Source text to position-aware tokens |
| `src/compiler/frontend/parser/` | Tokens to AST |
| `src/compiler/semantic/` | Name and type validation |
| `src/compiler/backend/codegen/` | AST to LLVM IR |
| `src/compiler/driver/` | CLI commands, project workflows and native toolchain orchestration |
| `src/runtime/` | Native runtime linked into generated applications |
| `stdlib/` | Modules imported as `std.io`, `std.fs`, and similar |
| `examples/templates/` | Starter sources used by `nexus new` |
| `examples/` | Runnable language and runtime examples |
| `tests/` | Unit and CLI regression tests |
| `docs/` | User and contributor guides |

## Dependency direction

```text
CLI → Driver → Frontend (Lexer → Parser → AST)
              → Semantic Analysis
              → Backend (LLVM IR)
              → Native Toolchain + Runtime/stdlib resolution
```

The AST is the shared boundary between parsing, semantic analysis and code generation. The runtime and standard library are resolved through explicit source-tree, environment, installed and portable-package paths. CI copies `src/runtime/` into a portable `runtime/` directory while keeping `stdlib/` and the packaged `project-templates/` directory at the package root.

## Change rules

- Keep the public `.nx` language syntax and CLI stable unless a language change is explicitly versioned.
- Keep the front end independent from platform-specific process launching.
- Give diagnostics stable error codes and preserve source file/line/column when the lexer or parser knows them.
- Add an executable regression test for every bug fix that can be expressed as a small program.
- Run CMake build, CTest, and Windows/Linux/macOS CI before merging architectural changes.

## Two-stage modernization

1. **Architecture and safety:** move source modules to their responsibility-based paths, remove the old source roots, update packaging and build files, and make syntax errors show a source excerpt and caret.
2. **Language and developer experience:** extend test coverage and tooling contracts before evolving generics, trait dispatch, closures/async lowering, package reproducibility and LSP features. Advanced language features must not be advertised as stable until their tests and implementation are complete.
