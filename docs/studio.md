# NEXUS Studio

NEXUS Studio is the desktop workspace for opening a project, editing one active source file, and running the NEXUS compiler without switching between separate tools.

<figure>
  <img src="assets/images/nexus-n-2026.svg" alt="NEXUS mark" width="64">
  <figcaption>Desktop workspace · preview release 0.3.0</figcaption>
</figure>

## What is included

- Project explorer for folders and files inside the selected workspace.
- Ace-based editor with NEXUS syntax highlighting, line numbers and standard editing shortcuts.
- Integrated terminal with process output, Stop, Clear and collapse controls.
- Compiler detection from `NEXUS_COMPILER`, PATH and common install locations; a selected compiler path is saved between launches.
- Direct Build and Run commands for the current `.nx` source file.
- A command palette (`Ctrl/Cmd+Shift+P`) for the most common workspace actions.

Studio does not bundle the compiler. Install NEXUS separately, then select its launcher if automatic detection does not find it. The official Windows distribution can use its `NEXUS-Windows.ps1` helper so the bundled toolchain and runtime DLLs are configured correctly.

## Download desktop builds

The desktop build workflow packages Windows (NSIS installer and portable executable), Linux (AppImage and DEB) and macOS (DMG and ZIP) builds.

- [Browse published Studio releases](https://github.com/fanxcz/nexus-lang/releases?q=studio)
- [Run the cross-platform build workflow](https://github.com/fanxcz/nexus-lang/actions/workflows/nexus-studio.yml)
- [Create a draft Studio release](https://github.com/fanxcz/nexus-lang/actions/workflows/studio-release.yml)

The release workflow creates a **draft** release with SHA-256 sidecars so the maintainer can review the platform packages before publishing. Until a draft is approved and published, the build workflow's artifacts are temporary CI outputs rather than permanent download links.

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

The generated files are written to `release/`. The cross-platform GitHub Actions workflow builds all three platform packages on their native runners.

## Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl/Cmd+O` | Open a project folder |
| `Ctrl/Cmd+N` | Create a source file |
| `Ctrl/Cmd+S` | Save the current file |
| `Ctrl/Cmd+Shift+P` | Open the command palette |
| `F7` | Build the current source file |
| `F5` | Build and run the current source file |

## Current boundaries

Studio currently shows one active editor tab at a time. It does not include a debugger, Git client, integrated LSP diagnostics panel or multi-file refactoring UI. The integrated terminal runs commands with the current operating-system user's permissions; only open projects and commands you trust. File access is restricted to the selected project folder, and individual files larger than 2 MB are not opened in the editor.

For language-aware completion, navigation and diagnostics, configure the [NEXUS VS Code extension](tooling/vscode-extension.md).
