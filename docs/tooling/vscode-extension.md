# VS Code extension for NEXUS

The repository contains the VS Code extension at `.github/project-tools/vscode-extension/`. It provides `.nx` syntax highlighting, snippets, and a client for the NEXUS Language Server.

## 1. Install NEXUS

Install the NEXUS compiler first and confirm it runs in a terminal:

```sh
nexus --version
nexus doctor
```

On Windows, install the NEXUS portable bundle, add it to PATH and reopen VS Code. If you keep NEXUS elsewhere, use the `nexus.serverPath` setting below.

## 2. Build the installable VSIX

Open a terminal at the root of your cloned `fanxcz/nexus-lang` repository and run:

```sh
cd .github/project-tools/vscode-extension
npm install
npx --yes @vscode/vsce package
```

This creates a VSIX whose name includes the extension package version (currently `nexus-language-2.1.0.vsix`) in the extension directory. You need Node.js/npm and network access for this one-time dependency download. The resulting VSIX includes its runtime dependency; it does not bundle the NEXUS compiler.

## 3. Install in VS Code

1. Open VS Code.
2. Press **Ctrl+Shift+P** (on macOS, **Cmd+Shift+P**).
3. Run **Extensions: Install from VSIX...**.
4. Select `.github/project-tools/vscode-extension/nexus-language-2.1.0.vsix`.
5. Reload VS Code when prompted.
6. Open any `.nx` source file. The language selector should display **NEXUS**.

You can also install it from the repository root with:

```sh
code --install-extension .github/project-tools/vscode-extension/nexus-language-2.1.0.vsix
```

The extension is not yet published on the VS Code Marketplace, so installing from VSIX is the available route.

## 4. Enable diagnostics and completion

The extension automatically starts `nexus lsp` for `.nx` files. If your NEXUS launcher is not on PATH, open VS Code Settings (JSON) and add:

```json
{
  "nexus.serverPath": "C:\\NEXUS\\nexus.cmd"
}
```

Use the **NEXUS Language Server** output channel to diagnose startup issues. The source tree currently implements diagnostics, completion, signature help, hover, definition/references/rename for indexed declarations, workspace symbols, formatting, and quick fixes. Resolution still has known type-inference and import-alias limits; a source-level debugger/debug adapter is not included.

## Troubleshooting

- **No colors:** reload VS Code and check the language selector says NEXUS.
- **No completion or diagnostics:** run `nexus lsp` in a terminal, verify PATH/`nexus.serverPath`, then check the **NEXUS Language Server** output channel.
- **VSIX packaging fails:** run `npm install` in the extension directory, then retry `npx --yes @vscode/vsce package`.
