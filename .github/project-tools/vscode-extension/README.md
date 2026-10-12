# NEXUS Language Support — VS Code extension

This extension registers `.nx` files as the **NEXUS** language. It includes syntax highlighting, bracket/comment behavior, snippets, and a language client that starts `nexus lsp` for live diagnostics, completion and basic declaration navigation.

The NEXUS compiler is not bundled inside the extension. Install the compiler separately and make sure the `nexus` command is available on PATH, or configure `nexus.serverPath` to the full path of the executable/launcher.

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
4. Select the generated `fanxcz.nexus-language-1.1.0.vsix` file.
5. Reload the window.
6. Open a `.nx` file; the language selector should show **NEXUS**.

Or install from a terminal:

```sh
code --install-extension fanxcz.nexus-language-1.1.0.vsix
```

## Custom compiler path

Add this to VS Code settings JSON if `nexus` is not on PATH:

```json
{
  "nexus.serverPath": "C:\\NEXUS\\nexus.cmd"
}
```

The LSP currently supports diagnostics, basic completion and basic declaration navigation. Full workspace indexing, references, rename, hover/type details, formatting and debugging are not implemented.
The extension is not yet published on the VS Code Marketplace.
