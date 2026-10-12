# Installation

## Pre-built Binaries

Download the latest release for your platform from [GitHub Releases](https://github.com/fanxcz/nexus-lang/releases).

### Linux (x86_64)

Portable and native compiler artifacts are provided through project releases when available. The built-in machine-code backend can produce x86-64 Linux ELF executables for its supported language subset; programs outside that subset need the configured LLVM/Clang fallback.

### Windows 10/11 (x86_64) — self-contained portable bundle

Download the [NEXUS Windows portable ZIP](../downloads.md) and extract the entire archive to a folder such as `C:\\NEXUS`. The bundle is designed to include the NEXUS compiler, its pinned fallback toolchain and target files, the NEXUS standard library, templates, runtime assets, and SQLite support.

For normal use with the supported packaged workflows, you should not need to separately install LLVM, CMake, Ninja, Visual Studio Build Tools, or the Windows SDK. Run `Check-NEXUS-Toolchain.bat` to verify the included backend can compile and launch a native program, then use the terminal commands below. Keep the `bin`, `toolchain`, `runtime`, `stdlib`, and `project-templates` folders together.

To add NEXUS to your user PATH:

1. Run `Install-NEXUS.bat` from the extracted folder. It checks the bundled backend and adds the folder to your user PATH.
2. Close and reopen PowerShell, Windows Terminal, or your editor terminal.
3. Verify the command:

   ```powershell
   nexus --version
   nexus doctor
   ```

4. Create or run a project:

   ```powershell
   nexus new HelloNexus --template cli
   cd HelloNexus
   nexus run
   ```

The PATH helper changes only the current user's PATH and does not require administrator privileges. Keep the extracted folder in the same location after adding it to PATH; run `Uninstall-NEXUS-Path.ps1` before deleting that folder.

### macOS (x86_64 / arm64)

Use the project release artifacts when published for your architecture. The native machine-code emitter is currently focused on x86-64 Windows/Linux. macOS builds use the configured Clang toolchain as required by the program and target.

## Building from Source
The compiler's `CMakeLists.txt` minimum is 3.20, but the checked-in `CMakePresets.json` uses schema version 6, which requires CMake 3.25+. If you have CMake 3.20–3.24, use explicit configure/build commands (`cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON` and `cmake --build build`) instead of `--preset` commands.


### Prerequisites

| Dependency | Minimum Version | Notes |
|------------|----------------|-------|
| CMake | 3.20 | Required for build system |
| Ninja | 1.10 | Recommended generator |
| Clang/LLVM | 18 | Required for LLVM fallback builds; not needed for all built-in native subset programs |
| SDL2 | 2.28 | Optional for building the graphics/audio-enabled runtime |
| C++20 Compiler | Clang 18 / GCC 13 / MSVC 19.40 | |

#### Ubuntu / Debian

```bash
sudo apt-get update && sudo apt-get install -y \
    cmake ninja-build clang-18 lld-18 \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

#### Arch Linux

```bash
sudo pacman -S cmake ninja clang lld sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

#### Fedora

```bash
sudo dnf install cmake ninja-build clang lld SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel
```

#### Windows (PowerShell)

```powershell
choco install cmake ninja llvm sdl2
# Or use vcpkg: vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf
```

#### macOS

```bash
brew install cmake ninja llvm sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

### Build Steps

```bash
git clone https://github.com/fanxcz/nexus-lang.git
cd nexus

cmake --preset=release
cmake --build --preset=release

# Unit, integration, backend, LSP, doctor, and self-test checks
ctest --test-dir build --output-on-failure

# Diagnose the local environment
./build/nexus doctor --json

# Install (optional)
sudo cmake --install build
```

### Verify Installation

```bash
nexus --version
nexus doctor
nexus self-test
```

## IDE Integration

### VS Code

Follow the [VS Code language extension guide](../tooling/vscode-extension.md). The project contains the extension source but does not yet publish a Marketplace extension. For basic workspace settings and build task generation, run `nexus editor` inside a NEXUS project. Configure `nexus lsp` as the language server for diagnostics and completion.

### CLion

Open the NEXUS compiler repository and select a supported configure preset from `CMakePresets.json`.

### Neovim / Vim

The NEXUS language server can be configured to run `nexus lsp` using the editor's standard LSP client. Compiler C++ development can use `clangd` after generating a compilation database with CMake.
 
## Troubleshooting

### "Clang not found"

Run `nexus doctor` to see whether the LLVM fallback was found. Supported built-in native-subset programs may still compile without a Clang executable. Programs outside the built-in subset require an available compiler via PATH, `NEXUS_CXX`, or the toolchain supplied by the platform package.

### SDL2 not found

CMake can build the compiler without SDL2, but the graphics/audio runtime will be limited. Install SDL2 development libraries for your platform and reconfigure the project if you need those features.

### Getting Help

- Check [GitHub Issues](https://github.com/fanxcz/nexus-lang/issues) for known problems.
- Run `nexus doctor --json` for script-friendly environment diagnostics.
- Open a [new issue](https://github.com/fanxcz/nexus-lang/issues/new/choose) with a reproducible example.
