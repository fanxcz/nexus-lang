# Language Server Protocol (LSP)

`nexus lsp` is a stdio-based JSON-RPC language server for NEXUS source files (`.nx`).

## Supported capabilities

- Initialize and clean shutdown.
- Full-document synchronization.
- Keyword and built-in completion items for common language constructs, standard-module imports and runtime functions.
- Hover documentation for the same built-in and keyword entries, returned as Markdown from the shared completion descriptions.
- Live diagnostics when a document is opened, changed, saved or closed.
- Basic go-to-definition for functions, structs, enums, variable bindings and simple function parameters declared in the current or another open document.
- Whole-document formatting via `textDocument/formatting`, using the same conservative formatter as `nexus fmt`.
- A maximum protocol message size of 16 MiB.

Diagnostics use the same lexer, parser and semantic analyzer as the compiler. Lexer errors use `NX1000`, parser errors use `NX1001`, and semantic errors use `NX2001`. Syntax errors include a line and column when available. Unknown-variable diagnostics point to the matching identifier token; other semantic errors currently point to the beginning of the document because semantic AST nodes do not yet preserve source spans.

The definition resolver is intentionally limited: it scans currently open documents and recognizes common declaration forms. It is not a full symbol index and does not navigate into files that are not open. Type-aware hover for user-defined symbols, code actions, inlay hints and full workspace indexing are not implemented. Current hover is documentation lookup for the built-in/keyword completion catalog. References and rename are supported only as lexical matches in currently open documents; formatting returns one whole-document edit and shares the CLI formatter so editor and command-line behavior stay aligned. Live diagnostics currently publish the first compiler error per document rather than attempting parser error recovery.

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


## Current reference and rename behavior

NEXUS supports `textDocument/references` and `textDocument/rename` for identifier tokens in currently open documents. The implementation uses the lexer, so occurrences inside comments and string literals are excluded. This is lexical matching by identifier spelling, not yet scope-aware semantic resolution; same-named identifiers from different scopes can be included. Closed files are not indexed. Check the proposed edits before accepting a rename in code with shadowed names.

## Verification

The cross-platform CTest suite sends initialize, didOpen, didChange, didClose, completion, hover, definition, references, rename, formatting, shutdown and exit messages to the actual server process:

```sh
ctest --test-dir build --output-on-failure
```

Future improvements include source spans for semantic diagnostics, multiple diagnostics per document, workspace-wide symbol indexing and formatter integration.
