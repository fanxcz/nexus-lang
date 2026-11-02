# NEXUS Language Support — VS Code extension

NEXUS Language Support registers `.nx` files as the **NEXUS** language. It provides syntax highlighting, snippets and a language client that starts `nexus lsp`.

## Language features

The current language server provides diagnostics, keyword/builtin completion, context-aware struct-field and enum-variant completion, signature help, hover, go-to-definition, references, rename, document outline, workspace symbol search, whole-document/range formatting and quick fixes for common unknown-name errors. Workspace symbol search is capped at 500 results.

Navigation and type-aware completion are best-effort rather than a full compiler-backed index. Imported aliases and complete type inference for arbitrary member-expression chains are not implemented. A debugger/debug adapter and inlay hints are not included.

## Install NEXUS first

The extension does not bundle the compiler. Install NEXUS separately and make sure the `nexus` command is available on PATH, or set `nexus.serverPath` to the full path of the executable/launcher.

## Build a VSIX

Requirements: Node.js/npm and network access for the first dependency download.

```sh
npm install
npx --yes @vscode/vsce package
```

The current package version is **2.2.0**. The resulting VSIX includes its production language-client dependency; it does not bundle the compiler.

## Install

1. Open VS Code.
2. Press **Ctrl+Shift+P** (macOS: **Cmd+Shift+P**).
3. Run **Extensions: Install from VSIX...**.
4. Select `fanxcz.nexus-language-2.2.0.vsix`.
5. Reload the window and open a `.nx` source file.

Or use:

```sh
code --install-extension fanxcz.nexus-language-2.2.0.vsix
```

The extension is not yet published on the VS Code Marketplace; installing from VSIX is the available route.

## Commands and settings

- **NEXUS: Show Language Server Output** opens the NEXUS Language Server output channel.
- `nexus.serverPath` configures a custom compiler executable/launcher when `nexus` is not on PATH.

Example settings JSON:

```json
{
  "nexus.serverPath": "C:\\NEXUS\\nexus.cmd"
}
```

For startup failures, run `nexus lsp` in a terminal and inspect the **NEXUS Language Server** output channel.
