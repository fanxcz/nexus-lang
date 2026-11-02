# NEXUS Studio

NEXUS Studio is the desktop workspace for opening a project, editing NEXUS source and running the compiler without switching between separate tools.

## Features

- Project explorer for folders and files inside the selected workspace.
- Ace editor with NEXUS syntax highlighting and keyboard-first editing.
- Integrated terminal with process output, Stop, Clear and collapse controls.
- Compiler detection through `NEXUS_COMPILER`, PATH and common install locations. The selected compiler path is remembered between launches.
- Direct Build and Run actions for the active `.nx` file.
- Searchable command palette (`Ctrl/Cmd+Shift+P`) for opening projects, creating files, choosing a compiler, refreshing the explorer and running Build/Run.
- Windows NSIS installer and portable executable; Linux AppImage/DEB; macOS DMG/ZIP packaging.

## Compiler setup

The compiler is not bundled inside the Studio installer. Install NEXUS separately. Studio checks `NEXUS_COMPILER`, PATH and common per-user install locations; otherwise click **Compiler** and select the installed launcher:

- Windows: `nexus.cmd` from the full installation/portable distribution, or `bin/nexus-core.exe` from the same distribution.
- Linux/macOS: the `nexus` executable.

The official Windows distribution includes `NEXUS-Windows.ps1` beside `nexus.cmd`. When this helper exists, Studio uses it so the bundled toolchain and runtime DLLs are configured automatically.

## Run from source

Requires Node.js 20+ and npm.

```sh
npm install
npm run check
npm start
```

## Build desktop packages

Run on the matching host platform:

```sh
npm run dist:win
npm run dist:linux
npm run dist:mac
```

Artifacts are written to `release/`. GitHub Actions also builds all three platform packages. To prepare a reviewable draft release with SHA-256 sidecars, run the [Studio release workflow](https://github.com/fanxcz/nexus-lang/actions/workflows/studio-release.yml). The workflow deliberately creates a draft release rather than publishing unreviewed binaries.

## Shortcuts

| Shortcut | Action |
|---|---|
| Ctrl/Cmd+O | Open project |
| Ctrl/Cmd+N | New file |
| Ctrl/Cmd+S | Save |
| Ctrl/Cmd+Shift+P | Command palette |
| F7 | Build |
| F5 | Build and run |

## Current boundaries

Studio currently shows one active editor tab at a time. It does not include a debugger, Git client, LSP diagnostics panel or multi-file refactoring UI. The integrated terminal runs commands with the current OS user's permissions; only run commands you trust. File operations are restricted to the opened project folder, and files larger than 2 MB are not opened in the editor.
