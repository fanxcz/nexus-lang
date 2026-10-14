# Language Server Protocol (LSP)

`nexus lsp` is a stdio-based JSON-RPC language server for NEXUS source files (`.nx`).

## Supported capabilities

- Initialize and clean shutdown, full-document synchronization, and diagnostics on open/change/save/close.
- Completion for keywords, built-ins and runtime functions, with context-aware suggestions for struct fields after `value.` and enum variants after `Enum::`.
- Hover for language/built-in documentation and parsed function/type declarations in the current valid buffer.
- Go-to-definition, references and rename for indexed declarations, references and scoped local bindings, including functions/types across discoverable workspace files.
- Whole-document formatting with `textDocument/formatting` and selected-line formatting with `textDocument/rangeFormatting`. Both use the same conservative formatter as `nexus fmt`.
- Quick-fix code actions for supported semantic mistakes, including close struct-field-name suggestions.
- A maximum protocol message size of 16 MiB.

## Workspace indexing

When a document is opened, NEXUS looks for the nearest `nexus.toml` and indexes project `.nx` files. The scanner excludes common generated or dependency directories such as `.git`, `build`, `dist`, `target`, `node_modules` and `.nexus`, with a 10,000-file limit. If no manifest is found, discovery is restricted to sibling `.nx` files; virtual URIs and root-level paths do not trigger a filesystem-wide scan.

The server caches discovered files and per-file lexical indexes. Unchanged indexes are reused. Open buffers take precedence over disk content; save, close and `workspace/didChangeWatchedFiles` notifications invalidate disk caches so later requests can refresh their view. Clients that support file watching should send these notifications to promptly reflect external additions, removals and edits.

## Symbol-resolution boundaries

The index tracks declarations, tokens and brace-delimited scopes. Navigation resolves top-level functions, structs and enums across discoverable workspace files, plus `let`/`const` bindings, loop variables and simple function parameters with scope awareness. Tests cover cross-file lookup, shadowing and rename without rewriting unrelated bindings.

Field and enum completion uses parsed source context; it is not yet a complete semantic index for all member expressions. Full navigation across imported aliases and robust inferred field types across files remain future work. Hover for user declarations currently parses the open buffer, so incomplete source can produce no declaration hover. Range formatting operates on the selected lines but computes indentation from the whole document.

## Diagnostics

The server uses the compiler lexer, parser and semantic analyzer. Diagnostic codes include `NX1000` for lexical errors, `NX1001` for syntax errors and `NX2001` for semantic errors. Parser recovery can report multiple independent top-level syntax errors; semantic analysis aggregates independent errors in declarations and function bodies. Parsed AST nodes retain source spans, so diagnostics for many syntax and semantic errors point to the corresponding source range rather than always to the start of the file.

## Launch

Configure your editor to start the NEXUS executable with the `lsp` argument. The server reads Content-Length-framed messages from standard input and writes protocol responses to standard output. Do not print debug logs to standard output because that corrupts the protocol stream.

```sh
nexus lsp
```

## VS Code

Install the [NEXUS VS Code extension](vscode-extension.md). It starts `nexus lsp` automatically for `.nx` files and sends server output to the **NEXUS Language Server** output channel.

## Helix

```toml
[[language]]
name = "nexus"
language-servers = ["nexus-lsp"]

[language-server.nexus-lsp]
command = "nexus"
args = ["lsp"]
file-types = ["nx"]
```

## Verification

The cross-platform CTest suite sends initialize, document lifecycle, completion, hover, definition, references, rename, whole-document formatting, range formatting, code-action, shutdown and exit messages to the actual server process. It also checks diagnostics, workspace lookup and scoped symbol behavior.

```sh
ctest --test-dir build --output-on-failure
```

Next LSP milestones are semantic navigation for imported aliases and a broader inferred type index, additional refactorings, and inlay hints.
