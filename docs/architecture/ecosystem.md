# NEXUS ecosystem architecture

This page documents the integration boundaries that exist in the repository today. It is intentionally descriptive: planned or experimental work is not presented as shipped functionality.

## User journey

```text
Documentation / Download Center
          |
          v
Compiler distribution -----> CLI / REPL / LSP
          |                       |
          v                       v
      NEXUS source          VS Code extension
          |                 (language client)
          v
    Lexer -> Parser -> AST -> Semantic analysis
                             |             |
                             v             v
                         LLVM IR      Native x86-64
                             |             |
                             +------.------+
                                    |
                                    v
                         Generated application
                                    |
                                    v
                         Runtime + stdlib
```

NEXUS Studio is a separate Electron desktop tool. It can open project folders, edit files with Ace, run terminal commands, select a compiler, and invoke the compiler's Build/Run commands. The current Studio implementation does not yet embed the NEXUS language server or provide full multi-document IDE workflows.

## Compiler and language services

| Area | Repository location | Current responsibility |
|---|---|---|
| CLI entry | `src/cli/` | Process entry point |
| Lexer | `src/compiler/frontend/lexer/` | Position-aware tokenization |
| Parser and AST | `src/compiler/frontend/parser/`, `src/compiler/ast/` | Recursive-descent parsing and shared syntax tree |
| Semantic analysis | `src/compiler/semantic/` | Name, binding, mutability and type validation |
| LLVM backend | `src/compiler/backend/codegen/` | Lowering the validated AST to LLVM IR |
| Built-in native backend | `src/compiler/backend/native/` | A documented x86-64 language subset; Windows PE and Linux ELF output |
| Driver | `src/compiler/driver/` | CLI, projects, packages, REPL, LSP and toolchain orchestration |
| Formatter | `src/compiler/format/` | Shared formatting implementation |

The LLVM path covers a broader portion of the language but relies on a compatible Clang/LLVM toolchain and target environment. The built-in native backend is deliberately narrower. Unsupported programs should receive a diagnostic rather than being silently compiled with different semantics.

The LSP transport and reusable helpers live in `src/compiler/driver/lsp/`. Protocol framing, UTF-16 text helpers, URI conversion and diagnostics collection are separate source units. The request-routing and workspace/indexing implementation remains concentrated in `server.cpp`; this is a known maintainability target, not evidence that every LSP feature is fully semantic or incremental.

The LSP currently offers diagnostics, completion, signature help, hover, definition, references, rename, formatting, document/workspace symbols and quick fixes within documented limitations. Rename preparation is exposed through `textDocument/prepareRename`. Workspace navigation is based on the server's lexical indexes and should not be confused with a complete compiler-backed symbol database.

## Build system

The root `CMakeLists.txt` is an orchestration entry point. It includes focused
modules in dependency order:

| Module | Responsibility | Required order |
|---|---|---|
| `cmake/NexusOptions.cmake` | Select C/C++ compilers and auto-detect Clang on Windows | Before `project()` |
| `cmake/NexusDependencies.cmake` | Discover optional SDL2 and SDL extensions | After `project()` |
| `cmake/NexusSources.cmake` | Define frontend, semantic, backend and driver source groups | Before target creation |
| `cmake/NexusTargets.cmake` | Configure compiler executable, include paths, definitions and link dependencies | After sources and compile options |
| `cmake/NexusInstall.cmake` | Install compiler, runtime, standard library, templates and assets | After compiler target creation |
| `cmake/NexusTests.cmake` | Register compiler, runtime, integration, regression and smoke tests | Under `NEXUS_BUILD_TESTS` |

Compiler selection remains before `project()` so CMake initializes platform
rules with the selected toolchain. Dependencies and targets are configured
after platform detection. The source groups are shared by the compiler and
unit-test targets, reducing the risk that one target silently misses a source
file included by the other.

The project keeps CMake 3.20 as its declared minimum and C++20 as its language
standard. Compiler overrides remain available through
`NEXUS_C_COMPILER` and `NEXUS_CXX_COMPILER`, with the existing Windows Clang
discovery behavior retained in the pre-project module.

## Runtime and standard library

The native runtime lives in `src/runtime/`. Its topic-specific implementation files are intentionally assembled through `runtime.cpp` into one translation unit so shared state and resource ownership remain coherent. Do not split these files into independent object files without first designing and testing the shared-state boundary.

The standard library lives separately in `stdlib/` and is imported by NEXUS programs. Runtime capabilities depend on the host build and optional native libraries. SDL-backed graphics and optional audio support are not available merely because the compiler executable itself builds successfully.

## Developer tools and applications

| Product | Source | Verified scope in the current tree |
|---|---|---|
| NEXUS CLI | `src/cli/`, `src/compiler/driver/` | Compiler commands, project workflows, formatter/linter, self-test and toolchain operations |
| REPL | `src/compiler/driver/repl/` | Interactive command/session implementation |
| LSP | `src/compiler/driver/lsp/` | Language-server protocol and lexical workspace services with documented limitations |
| VS Code extension | `.github/project-tools/vscode-extension/` | `.nx` language registration, TextMate grammar, snippets and a client that starts `nexus lsp`; compiler is installed separately |
| NEXUS Studio | `.github/project-tools/nexus-studio/` | Electron editor, project explorer, integrated terminal, compiler selection and Build/Run; LSP integration is not yet implemented in Studio |
| NEXUS Paint | `examples/nexus_paint.nx` | Raster canvas example with pencil, eraser, line/rectangle, fill, undo/redo and BMP load/save against the current graphics runtime |
| Other examples | `examples/` | Demonstrations and starter projects; each example's requirements are defined by its source and guide |

The VS Code extension has an independent version from the compiler. The extension launches the installed or configured compiler with the `lsp` argument; it does not bundle the compiler binary.

The scene snapshot API can now explicitly apply a loaded snapshot to the live ECS registry; that operation replaces the existing entity set and invalidates prior entity handles. This boundary is covered by a runtime CTest. NEXUS Paint is an application example written in NEXUS, not a separate cross-platform GUI framework. Its available drawing and image I/O features depend on the graphics runtime and the installed SDL support.

## Distribution and release boundaries

The download page is the source of user-facing installation guidance. Current Windows compiler, portable and Paint artifacts are validated and published by the Pages packaging workflow. The VS Code VSIX is versioned independently. NEXUS Studio has cross-platform packaging configuration and source-build instructions; its existence in the repository does not imply that a ready-made installer is published for every operating system.

A release must verify the actual artifact, checksum, platform and dependency set before adding a download link. A CI host build is not by itself proof that an installer, portable bundle or every runtime feature is available on that host.

## Support labels

Use these labels consistently in docs, website pages and release notes:

- **Implemented:** present in source and covered by an executable test or a documented smoke check.
- **Constrained:** implemented for a defined subset, host, backend or runtime dependency set.
- **Experimental:** present but not yet a supported production capability.
- **Planned:** not available in the current source tree or release artifacts.
- **Deprecated:** retained for compatibility but scheduled for removal under an explicit migration policy.

A directory name, prototype, roadmap item or UI label does not change a feature's support status. When in doubt, link to the implementation and its tests.

## Change and validation rules

1. Preserve the public `.nx` syntax and existing project manifests unless a compatibility change is explicitly designed.
2. Keep frontend and semantic contracts shared by the backends; do not claim backend parity without comparing executable behavior.
3. Preserve the runtime translation-unit model until shared state and ownership are understood.
4. Validate project and package paths, including symlinks and traversal attempts.
5. Test the compiler, runtime, Studio security helpers, extension metadata and website build independently.
6. Publish only artifacts that were actually produced and validated.

See the [compiler pipeline](compiler-pipeline.md), [repository layout](repository-layout.md), [platform support matrix](../development/platform-support.md), and [release downloads](../downloads.md).
