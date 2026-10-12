# Downloads

Download the Windows portable bundle to compile and run native NEXUS programs without installing LLVM, Clang, Visual Studio Build Tools, or a separate Windows SDK. The ZIP includes the NEXUS compiler, a pinned Zig-based C++ compiler/linker, runtime DLLs for graphics/audio/database functionality, standard modules, templates, and launchers.

<div class="grid cards" markdown>

-   :material-microsoft-windows: **NEXUS for Windows 10/11 x64**

    Portable folder with a bundled native backend and runtime dependencies.

    [Download Windows ZIP](assets/downloads/NEXUS_Windows_Portable.zip){ .md-button .md-button--primary }

    [SHA-256 checksum](assets/downloads/NEXUS_Windows_Portable.zip.sha256.txt)

-   :material-code-tags: **NEXUS language extension for VS Code**

    Syntax highlighting, snippets, diagnostics and completion for `.nx` source files. Install it from the extension archive in VS Code.

    [Download VSIX](assets/downloads/NEXUS-Language-1.1.0.vsix){ .md-button .md-button--primary }

    [SHA-256 checksum](assets/downloads/NEXUS-Language-1.1.0.vsix.sha256.txt)

-   :material-laptop: **Linux and macOS compiler builds**

    The release workflow is ready to package the compiler, standard modules, templates, examples, and pinned Zig C++ compiler/linker. Its first downloadable archives appear here after a version tag is published.

    Planned archives cover Linux x86-64, macOS Apple Silicon, macOS Intel, and Windows x64. The direct machine-code backend is currently limited to x86-64 Linux and Windows; macOS uses the bundled C++/LLVM path.

    [Browse GitHub Releases](https://github.com/fanxcz/nexus/releases){ .md-button .md-button--primary }

-   :material-source-repository: **Full source**

    C++20 compiler source, runtime, standard modules, tests, examples, templates, and documentation.

    [Open GitHub repository](https://github.com/fanxcz/nexus){ .md-button }

</div>

## Windows installation

1. Download and extract the ZIP to a permanent folder, for example `C:\\NEXUS`. Do not run it from inside the ZIP viewer.
2. Double-click `Install-NEXUS.bat`. It verifies the bundled native toolchain and adds the NEXUS folder to your user `PATH` without administrator rights.
3. Close and reopen PowerShell/CMD/Windows Terminal, then use `nexus --version`, `nexus doctor`, `nexus "C:\\path\\to\\hello.nx"`, or `nexus build "C:\\path\\to\\hello.nx"` from any folder.
4. Alternatively, drag a `.nx` file onto `Run-NEXUS.bat` to compile and run it, or onto `Build-NEXUS.bat` to create an `.exe` beside the source.

## What is bundled

- NEXUS command-line compiler executable.
- Zig C++ compiler/linker and its required libraries.
- SDL2, SDL2_image, SDL2_ttf, SDL2_mixer and SQLite runtime DLLs.
- NEXUS standard library, project templates, example program and launch scripts.

No additional compiler, linker, graphics runtime or database DLL installation is required for the supported Windows x64 portable workflow. Windows itself provides the operating-system APIs required to launch native programs.

## Build from source

Building the NEXUS compiler from its C++ source is a separate developer workflow and still requires a C++20 toolchain, CMake, Ninja, and the optional SDL2 development packages. Those are build-time prerequisites for contributors, not runtime installation requirements for users of the portable ZIP.

## Integrity

The SHA-256 file lets you verify that your ZIP matches the package published with this source version. After downloading, calculate the hash in PowerShell:

```powershell
Get-FileHash .\\NEXUS_Windows_Portable.zip -Algorithm SHA256
```

Compare the `Hash` value with the checksum file.

## Platform note

The website's Windows portable ZIP is the currently available ready-to-use package. The release workflow produces separate compiler archives for Linux x86-64, macOS Apple Silicon, macOS Intel, and Windows x64 after a version tag is published. Each archive bundles a matching Zig-based C++ toolchain.

The direct machine-code backend currently emits x86-64 Windows and Linux binaries for its documented language subset. The bundled Zig wrapper supplies the broader LLVM compilation path on all released platforms. Optional graphics/audio integrations may still depend on platform runtime libraries.
