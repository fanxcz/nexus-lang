# NEXUS Studio 3.0.0

NEXUS Studio is the cross-platform desktop IDE for NEXUS projects. It combines a project explorer, multi-file editor, compiler workflow, diagnostics list and integrated terminal in one workspace.

## IDE features

- **Project explorer:** open a folder, browse nested directories, refresh the tree and filter filenames.
- **Multi-tab editor:** keep several files open, switch between tabs, preserve unsaved buffers and close tabs independently.
- **NEXUS editing:** Ace editor with NEXUS syntax highlighting, line numbers, cursor position, indentation and familiar keyboard shortcuts.
- **Problems panel:** compiler errors and warnings that include source locations are collected from build output; select an entry to open its file and jump to the reported line and column.
- **Build and Run:** detect a compiler through `NEXUS_COMPILER`, PATH and common install locations, or select its executable manually.
- **Integrated terminal:** run commands from the project root, inspect stdout/stderr, stop the current process, clear output and collapse the panel.
- **Command palette:** search common project, editor, compiler, terminal and diagnostic actions with `Ctrl/Cmd+Shift+P`.

Studio is a desktop application built with Electron. It is not a web-based IDE and its desktop file and process operations run locally on the current operating-system account.

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

## Download Studio 3.0.0

- [Windows x64 installer (.exe)](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Windows-x64.exe) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Windows-x64.exe.sha256.txt)
- [Linux x86_64 AppImage](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-x86_64.AppImage) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-x86_64.AppImage.sha256.txt)
- [Linux Debian/Ubuntu package (.deb)](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-amd64.deb) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-amd64.deb.sha256.txt)
- [macOS Apple Silicon disk image (.dmg)](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-macOS-arm64.dmg) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-macOS-arm64.dmg.sha256.txt)
- [All release files and notes](https://github.com/fanxcz/nexus-lang/releases/tag/studio-v3.0.0)

## Build desktop packages

Run on the matching host platform:

```sh
npm run dist:win
npm run dist:linux
npm run dist:mac
```

Artifacts are written to `release/`. The [cross-platform build workflow](https://github.com/fanxcz/nexus-lang/actions/workflows/nexus-studio.yml) builds all three platform packages. The [Studio release workflow](https://github.com/fanxcz/nexus-lang/actions/workflows/studio-release.yml) verifies their SHA-256 sidecars and publishes the official release.

## Shortcuts

| Shortcut | Action |
|---|---|
| Ctrl/Cmd+O | Open project |
| Ctrl/Cmd+N | Create file |
| Ctrl/Cmd+S | Save active file |
| Ctrl/Cmd+W | Close active tab |
| Ctrl/Cmd+P | Filter project filenames |
| Ctrl/Cmd+Shift+P | Command palette |
| F7 | Build active source |
| F5 | Build and run active source |
| F8 | Toggle Problems panel |

## Current boundaries

Studio 3.0.0 includes multi-file editing, project navigation, a compiler-driven Problems panel and a local terminal. It does **not** currently include a graphical debugger, Git client, integrated language-server protocol client, symbol index or multi-file refactoring engine. The Problems panel parses source locations emitted by the compiler; it is not a replacement for a full LSP diagnostics engine.

The terminal runs commands with the current OS user's permissions; only run commands you trust. File operations are restricted to the opened project folder, and files larger than 2 MB are not opened in the editor.
