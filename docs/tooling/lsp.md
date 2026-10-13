# Language Server Protocol (LSP)

\`nexus lsp\` is a stdio-based JSON-RPC language server for NEXUS source files (\`.nx\`).

## Supported capabilities

- Initialize and clean shutdown, plus full-document synchronization.
- Keyword and built-in completion items for common language constructs, standard-module imports and runtime functions.
- Hover documentation for built-ins/keywords, plus parsed signatures and field/variant descriptions for user-defined declarations in a syntactically valid open file.
- Live diagnostics when a document is opened, changed, saved or closed. The server returns the first compiler error it can identify for each document.
- Go-to-definition, references and rename using a cached per-file lexical index. The index tracks declarations, tokens and brace-delimited scopes.
- Workspace lookup for project \`.nx\` files that are not currently open in the editor. Open document buffers override the on-disk copy.
- Whole-document formatting via \`textDocument/formatting\`, using the same conservative formatter as \`nexus fmt\`.
- A maximum protocol message size of 16 MiB.

## Workspace indexing

When a document is opened, NEXUS looks for the nearest \`nexus.toml\` and scans the project for \`.nx\` files. The scanner excludes common generated or dependency directories such as \`.git\`, \`build\`, \`dist\`, \`target\`, \`node_modules\` and \`.nexus\`, with a 10,000-file limit. If no manifest is found, discovery is restricted to sibling \`.nx\` files; virtual URIs and root-level paths do not trigger a filesystem-wide scan.

The server caches the discovered source files per workspace and caches a lexical index for each file. Unchanged file indexes are reused. Open buffers take precedence over disk content; save, close, and \`workspace/didChangeWatchedFiles\` notifications invalidate the disk cache so later requests can refresh their view. Clients that support file watching should send those notifications to promptly reflect external additions, removals, and edits.

## Symbol-resolution boundaries

References, rename and go-to-definition resolve local \`let\`/\`const\` bindings, loop variables and simple function parameters with brace-based scope awareness. Top-level functions, structs and enums are resolved across discoverable workspace files. Shadowed local variables in nested blocks are kept separate, and the smoke tests cover cross-file lookup and scoped rename.

This remains a lexical workspace index, not a complete semantic/type index. Struct fields, member accesses, enum variants, imported aliases and inferred expression types do not yet have full semantic resolution. Hover for a user-defined declaration still relies on parsing the current open file; incomplete buffers may therefore return no declaration hover. There is no range formatter, code actions or inlay hints yet.

## Diagnostics

Diagnostics use the compiler's lexer, parser and semantic analyzer. Lexer errors use \`NX1000\`, parser errors use \`NX1001\`, and semantic errors use \`NX2001\`. Syntax errors include a line and column when available. Unknown-variable diagnostics point to the matching identifier token; many other semantic errors currently point to the beginning of the document because semantic AST nodes do not yet preserve source spans. Multi-error parser recovery is not implemented.

## Launch

Configure your editor to start the NEXUS executable with the \`lsp\` argument. The server reads Content-Length-framed messages from standard input and writes protocol responses to standard output. Do not print debug logs to standard output because that corrupts the protocol stream.

\`\`\`sh
nexus lsp
\`\`\`

## VS Code

Install the [NEXUS VS Code extension](vscode-extension.md). It starts \`nexus lsp\` automatically for \`.nx\` files and sends server output to the **NEXUS Language Server** output channel.

## Helix

\`\`\`toml
[[language]]
name = "nexus"
language-servers = ["nexus-lsp"]

[language-server.nexus-lsp]
command = "nexus"
args = ["lsp"]
file-types = ["nx"]
\`\`\`

## Verification

The cross-platform CTest suite sends initialize, didOpen, didChange, didClose, completion, hover, definition, references, rename, formatting, shutdown and exit messages to the actual server process. It includes a temporary on-disk project to test unopened-file navigation and references, and a shadowing case to ensure renaming an inner local variable does not rewrite same-named variables in other scopes.

\`\`\`sh
ctest --test-dir build --output-on-failure
\`\`\`

Next LSP milestones are deeper semantic resolution for fields/imports/enum variants, source spans and multiple diagnostics per document, plus client capability negotiation and range formatting.
