# Editor Integration

NEXUS has two editor-related workflows. The `editor` command generates workspace files; the graphical editor template is experimental.

## Generate VS Code project settings

From a NEXUS project directory:

```sh
nexus editor
```

This writes:

- `.vscode/settings.json` — associates `.nx` files with the `nexus` language ID and sets indentation defaults.
- `.vscode/tasks.json` — adds a basic `NEXUS Build` task which runs `nexus build --release`.

Install the [NEXUS VS Code language extension](vscode-extension.md) to get syntax colors, bracket behavior, snippets, and access to the NEXUS Language Server. The extension is source-only in this repository and is not currently on the VS Code Marketplace.

## Graphical editor example

```sh
nexus editor --gui
```

This experimental path compiles and launches the bundled editor example. It can require the LLVM/Clang fallback and graphics runtime, depending on which constructs are used and how NEXUS was packaged. It is not yet a full replacement for a mature IDE.

## Language server

The extension starts `nexus lsp` for live compiler diagnostics, completion, signature help, hover, go-to-definition, references, rename, document/range formatting, quick fixes, and workspace-symbol search. Workspace indexing uses lexical declarations and known inferred types rather than a complete semantic model; import aliases and some cross-file field inference remain limited. A source debugger/debug adapter and format-on-save workflow are not included.

## Limits

The CLI has no command to open a specific file at a line, pipe-to-editor mode, or configurable theme/keybinding system. Consult the [CLI command reference](commands.md) for supported commands and the [roadmap](../development/roadmap.md) for planned improvements.
