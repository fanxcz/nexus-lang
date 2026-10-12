# Downloads

Choose the official NEXUS package for your system. The Windows x64 package is assembled and tested by GitHub Actions from this repository. Downloads are published to GitHub Pages only when the packaging and documentation workflow succeeds.

<div class="grid cards" markdown>

-   :material-microsoft-windows: **NEXUS Setup for Windows 10/11 x64**

    Recommended for most Windows users. Installs NEXUS for the current user, offers Start menu and desktop shortcuts, and can add the launcher to the user PATH. No administrator rights are required.

    [Download NEXUS-Setup-1.1.0.exe](assets/downloads/NEXUS-Setup-1.1.0.exe){ .md-button .md-button--primary }

    [Installer SHA-256](assets/downloads/NEXUS-Setup-1.1.0.exe.sha256.txt)

-   :material-archive: **Portable Windows x64 package**

    Includes the compiler launcher and executable, bundled Zig-based C++ toolchain, runtime DLLs, standard library, templates, examples, and helper scripts. Extract to a permanent folder before running it.

    [Download Windows ZIP](assets/downloads/NEXUS_Windows_Portable.zip){ .md-button }

    [ZIP SHA-256](assets/downloads/NEXUS_Windows_Portable.zip.sha256.txt)

-   :material-code-tags: **NEXUS language extension for VS Code**

    Syntax highlighting, snippets, diagnostics, and completion for `.nx` source files.

    [Download VSIX](assets/downloads/NEXUS-Language-1.1.0.vsix){ .md-button }

    [VSIX SHA-256](assets/downloads/NEXUS-Language-1.1.0.vsix.sha256.txt)

-   :material-monitor: **NEXUS Studio — Windows x64 desktop IDE**

    Standalone desktop editor with a project explorer, `.nx` editor, compiler selection, Build/Run buttons, and output terminal. Install the NEXUS compiler separately and select it inside Studio.

    [Download NEXUS Studio Windows EXE](https://raw.githubusercontent.com/fanxcz/nexus-lang/main/.github/project-tools/nexus-studio/downloads/NEXUS-Studio-Windows-x64.exe){ .md-button }

    [Studio installer SHA-256](https://raw.githubusercontent.com/fanxcz/nexus-lang/main/.github/project-tools/nexus-studio/downloads/NEXUS-Studio-Windows-x64.exe.sha256)

-   :material-palette: **NEXUS Paint — native drawing app for Windows x64**

    A desktop raster editor written in the NEXUS language. It includes a brush, eraser, line and rectangle tools, fill, colour palette, adjustable brush size, undo/redo and BMP save/load. The portable ZIP is recommended because it bundles the SDL runtime DLLs and editable .nx source.

    [Download NEXUS Paint portable ZIP](assets/downloads/NEXUS-Paint-Windows-x64.zip){ .md-button .md-button--primary }

    [Download EXE only (requires DLLs from the ZIP)](assets/downloads/NEXUS-Paint-Windows-x64.exe)

    [EXE SHA-256](assets/downloads/NEXUS-Paint-Windows-x64.exe.sha256.txt) · [ZIP SHA-256](assets/downloads/NEXUS-Paint-Windows-x64.zip.sha256.txt)

    [View the NEXUS source](https://github.com/fanxcz/nexus-lang/blob/main/examples/nexus_paint.nx)

-   :material-laptop: **Linux and macOS compiler builds**

    Linux x86-64 and macOS Apple Silicon/Intel archives are built by the tagged-release workflow. They become downloadable when a version-tag release has completed successfully; check the official Releases page for currently published assets.

    [Browse GitHub Releases](https://github.com/fanxcz/nexus-lang/releases){ .md-button }

-   :material-source-repository: **Source code**

    Compiler, standard library, runtime, tests, examples, templates, and documentation.

    [Open the repository](https://github.com/fanxcz/nexus-lang){ .md-button }

</div>

## Install on Windows

1. Download `NEXUS-Setup-1.1.0.exe` from this page after the artifact is available.
2. Run the installer and follow the prompts. It installs under `%LOCALAPPDATA%\\Programs\\NEXUS` by default.
3. Select the optional shortcuts and PATH integration you want.
4. Close and reopen PowerShell, then verify:

```powershell
nexus --version
nexus doctor
nexus self-test
```

If the command is not found, first open a new terminal. You can test the installed launcher directly:

```powershell
& "$env:LOCALAPPDATA\\Programs\\NEXUS\\nexus.cmd" --version
```

The installer is not code-signed. Only run it if you downloaded it from this repository's official GitHub Pages output or GitHub Actions artifact and are comfortable with the unsigned-app warning Windows may display.

## Portable installation

1. Extract the ZIP to a permanent writable folder, for example `C:\\NEXUS`.
2. Double-click `Install-NEXUS.bat` if you want to add that folder to your user PATH.
3. Open a new terminal and run `nexus --version`, `nexus doctor`, or `nexus self-test`.

The portable package is designed to avoid requiring a separately installed LLVM/Clang toolchain for its bundled Windows workflow. Feature availability still depends on the selected compiler backend and target.

## Integrity

Use PowerShell to calculate the checksum of a downloaded file:

```powershell
Get-FileHash .\\NEXUS-Setup-1.1.0.exe -Algorithm SHA256
Get-FileHash .\\NEXUS_Windows_Portable.zip -Algorithm SHA256
```

Compare each hash with the matching `.sha256.txt` file published alongside the download.
