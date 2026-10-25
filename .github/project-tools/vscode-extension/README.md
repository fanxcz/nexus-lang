# NEXUS Language Support — VS Code extension

This extension registers `.nx` files as the **NEXUS** language. It includes syntax highlighting, bracket/comment behavior, snippets, and a language client that starts `nexus lsp` for language-server features.

## Features

The current LSP implementation provides diagnostics, keyword/builtin completion, context-aware struct-field and enum-variant completion, signature help, hover, go-to-definition, references, rename, document outline, workspace symbol search, whole-document and range formatting, and quick-fix suggestions for common unknown-name errors. Workspace symbol search is capped at 500 results.

Navigation and type-aware completion are best-effort rather than a full compiler-backed index. Imported aliases, complete type inference for arbitrary member-expression chains, debugging integration, and inlay hints are not implemented. See the [LSP guide](../../../docs/tooling/lsp.md) for details.

The compiler is not bundled inside the extension. Install NEXUS separately and make sure the `nexus` command is available on PATH, or configure `nexus.serverPath` to the full path of the executable/launcher.

## Build a VSIX

Requirements: Node.js/npm and network access for the first dependency download.

```sh
npm install
npx --yes @vscode/vsce package
```

This creates a `.vsix` file in this directory. The production dependency `vscode-languageclient` is included in the VSIX so users do not need to run npm after installing it.

## Install in VS Code

1. Open VS Code.
2. Press **Ctrl+Shift+P** (macOS: **Cmd+Shift+P**).
3. Run **Extensions: Install from VSIX...**.
4. Select the generated `fanxcz.nexus-language-2.0.2.vsix` file.
5. Reload the window.
6. Open a `.nx` file; the language selector should show **NEXUS**.

Or install from a terminal:

```sh
code --install-extension fanxcz.nexus-language-2.0.2.vsix
```

## Custom compiler path

Add this to VS Code settings JSON if `nexus` is not on PATH:

```json
{
  "nexus.serverPath": "C:\\NEXUS\\nexus.cmd"
}
```

The extension is not yet published on the VS Code Marketplace.
