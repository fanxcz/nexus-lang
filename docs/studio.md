# NEXUS Studio 3.0.0

NEXUS Studio is the cross-platform desktop IDE for opening projects, editing multiple NEXUS files, building programs and reviewing compiler output in one workspace.

![NEXUS mark](assets/images/nexus-n-2026.svg){ width="64" }

*Desktop IDE · release 3.0.0*

## IDE workspace

- **Project explorer:** browse nested folders, refresh project contents and filter filenames.
- **Multi-tab editing:** open multiple files, switch tabs without losing unsaved buffers, and save the active file or every open file.
- **NEXUS editor:** Ace-based editing with NEXUS syntax highlighting, line numbers, indentation, cursor position and familiar editing shortcuts.
- **Problems panel:** collect compiler errors and warnings that include source locations; select a diagnostic to navigate to its file, line and column.
- **Compiler workflow:** detect NEXUS through `NEXUS_COMPILER`, PATH or common install locations, or select a compiler path manually.
- **Integrated terminal:** run project commands, inspect process output and stop a running process.
- **Command palette:** search common workspace actions with `Ctrl/Cmd+Shift+P`.

Studio is an Electron desktop application; file access and terminal commands run locally under the current OS account. The compiler is installed separately rather than bundled in the Studio app. On Windows, the official distribution's `NEXUS-Windows.ps1` helper configures its bundled toolchain and runtime DLLs.

## Download desktop builds

The official [NEXUS Studio 3.0.0 GitHub Release](https://github.com/fanxcz/nexus-lang/releases/tag/studio-v3.0.0) includes native platform packages and SHA-256 sidecars.

- [Windows x64 installer (.exe)](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Windows-x64.exe) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Windows-x64.exe.sha256.txt)
- [Linux x86_64 AppImage](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-x86_64.AppImage) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-x86_64.AppImage.sha256.txt)
- [Linux Debian/Ubuntu package (.deb)](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-amd64.deb) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-Linux-amd64.deb.sha256.txt)
- [macOS Apple Silicon disk image (.dmg)](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-macOS-arm64.dmg) · [SHA-256](https://github.com/fanxcz/nexus-lang/releases/download/studio-v3.0.0/NEXUS-Studio-3.0.0-macOS-arm64.dmg.sha256.txt)
- [All release files and notes](https://github.com/fanxcz/nexus-lang/releases/tag/studio-v3.0.0)
- [Cross-platform build workflow](https://github.com/fanxcz/nexus-lang/actions/workflows/nexus-studio.yml)

## Build from source

Requires Node.js 20+ and npm.

```sh
cd .github/project-tools/nexus-studio
npm install
npm run check
npm start
```

Build packages on the matching host platform:

```sh
npm run dist:win
npm run dist:linux
npm run dist:mac
```

The generated files are written to `release/`. The cross-platform GitHub Actions workflow builds all three platform packages on native runners.

## Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl/Cmd+O` | Open a project folder |
| `Ctrl/Cmd+N` | Create a source file |
| `Ctrl/Cmd+S` | Save the active file |
| `Ctrl/Cmd+W` | Close the active editor tab |
| `Ctrl/Cmd+P` | Filter project filenames |
| `Ctrl/Cmd+Shift+P` | Open the command palette |
| `F7` | Build the current source file |
| `F5` | Build and run the current source file |
| `F8` | Toggle the Problems panel |

## Current boundaries

Studio 3.0.0 includes multi-tab editing, project navigation, build/run actions, compiler-output diagnostics and an integrated terminal. It does not include a graphical debugger, Git client, full language-server protocol client, symbol index or multi-file refactoring engine. Compiler diagnostics appear when the output includes a file and source location; this panel is not a full LSP diagnostics implementation.

The terminal runs with the current operating-system user's permissions; only open projects and run commands you trust. File access is restricted to the selected project folder, and individual files larger than 2 MB are not opened in the editor.

For language-aware completion, navigation and richer diagnostics, configure the [NEXUS VS Code extension](tooling/vscode-extension.md).
