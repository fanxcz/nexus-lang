# NEXUS Studio

A small desktop IDE for the NEXUS programming language: project explorer, editor, terminal, compiler selection, and working Build/Run commands.

## Features
- Open a project folder, browse files, create and save .nx source files.
- Build the current .nx file into a native program or compile and run it.
- Keep multiple files open in tabs with independent unsaved buffers; switch between documents without discarding edits.
- Start the installed compiler's LSP process for the open project; show live diagnostics, request completions with Ctrl+Space, display symbol hover information, and navigate to definitions with F12.
- Select a compiler once; its path is remembered after restarting the IDE.
- Integrated terminal with Clear, Stop and Hide/Show controls.
- Shortcuts: Ctrl/Cmd+O (open folder), Ctrl/Cmd+S (save), Ctrl/Cmd+N (new file), F7 (build), F5 (run).
- Windows, Linux and macOS packaging configuration.

## Compiler setup
The compiler is not bundled inside the IDE installer. Install the NEXUS distribution separately, then click **Compiler…** and select:
- Windows: nexus.cmd from the full NEXUS installation/portable folder, or bin/nexus-core.exe inside that same folder.
- Linux/macOS: the nexus executable.

The official Windows distribution includes NEXUS-Windows.ps1 beside nexus.cmd. When this helper exists, the IDE uses it so the bundled toolchain and runtime DLLs are configured automatically. For a standalone CLI executable, the IDE invokes the documented nexus build or nexus run arguments. Automatic detection checks NEXUS_COMPILER, PATH, and common Windows install locations.

## Run from source
Requires Node.js 20+ and npm.

    npm install
    npm start

## Build desktop packages

    npm run dist:win
    npm run dist:linux
    npm run dist:mac

Artifacts are written to release/. GitHub Actions builds native packages on Windows, Linux and macOS runners.

## Security
The renderer uses context isolation and a limited preload API. File operations are restricted to the opened project folder. Terminal commands run with the current user's permissions; execute only commands you trust.

## Current boundaries

NEXUS Studio supports multiple open document tabs and retains unsaved buffers when switching between files. It starts the separately installed NEXUS language server and integrates live diagnostics, completion, hover information, and go-to-definition for the active .nx document. Signature help, references, rename, formatting actions, and debugging are not yet integrated into the Studio UI. Use the VS Code extension for those additional language-client features.

The integrated terminal executes commands with the current user's permissions. Only run commands and project code that you trust. File reads and writes made through Studio's project API are confined to the opened project folder; shell commands are intentionally not sandboxed.
