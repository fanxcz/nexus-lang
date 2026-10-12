# NEXUS

**NEXUS is an open-source programming language and native runtime for applications, tools, and games.** Its compiler is written in C++20. It includes a built-in x86-64 native backend for a documented subset of the language and an LLVM-based compilation path for broader support.

[![CI](https://github.com/fanxcz/nexus-lang/actions/workflows/build.yml/badge.svg)](https://github.com/fanxcz/nexus-lang/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

**Repository:** https://github.com/fanxcz/nexus-lang  
**Documentation and downloads:** https://fanxcz.github.io/nexus-lang/  
**Windows installer:** download `NEXUS-Setup-1.1.0.exe` from the [project downloads page](https://fanxcz.github.io/nexus-lang/downloads/) or from a successful [GitHub Actions Pages run](https://github.com/fanxcz/nexus-lang/actions/workflows/pages.yml).

> The website URL above is the GitHub Pages project URL for this repository. Do not use the old `fanxcz.github.io/nexus/` address; that belongs to a different repository/site.

## Current status

NEXUS is under active development. The command-line compiler, built-in native backend, project templates, standard library, runtime foundations, formatter/linter, diagnostics, and tests are present in this repository. Some advanced language and runtime features remain experimental; consult the documentation and tests before relying on a feature in production.

The built-in native backend supports a defined subset (including integer, boolean and string values, functions, control flow, printing/input, and selected string operations) and can emit x86-64 Windows PE or Linux ELF executables without a separately installed LLVM/Clang toolchain. Programs outside that subset may require the LLVM compilation path and its toolchain.

## Install on Windows 11

1. Open the [NEXUS downloads page](https://fanxcz.github.io/nexus-lang/downloads/) or the [Pages workflow runs](https://github.com/fanxcz/nexus-lang/actions/workflows/pages.yml).
2. Download `NEXUS-Setup-1.1.0.exe` only from an official project artifact.
3. Run the installer. It installs for the current user under `%LOCALAPPDATA%\Programs\NEXUS`; administrator rights are not required.
4. Select the options you want: Start menu shortcuts, desktop shortcut, add NEXUS to the user PATH, and optional `.nx` context-menu integration.
5. Open a **new** PowerShell window and verify:

```powershell
nexus --version
nexus doctor
nexus self-test
```

If `nexus` is not recognized, reopen the terminal first. If it still is not recognized, run the installed launcher directly:

```powershell
& "$env:LOCALAPPDATA\Programs\NEXUS\nexus.cmd" --version
```

The Windows installer is unsigned, so Windows SmartScreen may show a warning. Verify that the file came from this repository's official GitHub Actions/Pages output before deciding whether to run it. Never download installers from third-party mirrors.

### Portable Windows package

The [portable package](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS_Windows_Portable.zip), when published by a successful Pages workflow, contains the compiler launcher, compiler executable, bundled Zig-based C++ toolchain, standard library, templates, examples, and runtime DLLs assembled by the packaging workflow. Extract the ZIP to a writable folder and run `nexus.cmd` from that folder. Use the installer for normal system integration.

## Quick start

Create a project from a template:

```powershell
nexus new MyGame --template game
cd MyGame
nexus build
nexus run
```

Try a small source file named `hello.nx`:

```nx
fn main() {
    print("Hello from NEXUS!")
}
```

Run it with:

```powershell
nexus run hello.nx
```

For programs supported by the built-in backend, you can explicitly select it:

```powershell
nexus build hello.nx -o hello.exe --backend native
nexus run hello.nx --backend native
```

Use `nexus explain-backend hello.nx` to inspect backend selection and `nexus doctor` to check the local setup.

## Common commands

```text
nexus --version
nexus new MyApp --template cli
nexus init
nexus check hello.nx
nexus build
nexus run
nexus clean
nexus info
nexus doctor
nexus self-test
nexus fmt
nexus fmt --check
nexus lint
nexus doc
nexus bench --runs 5
nexus editor
nexus lsp
```

Exact options can vary by command and current implementation. Use `nexus <command> --help` where supported and consult the [command reference](https://fanxcz.github.io/nexus-lang/tooling/commands/).

## Standard library and runtime

The repository includes standard modules for common operations, including I/O, filesystem access, networking, JSON utilities, database access, memory helpers, and cryptographic utilities. Runtime foundations include SDL-based 2D rendering, optional OpenGL 3D, audio, UI, ECS, scenes, particles, and physics components. Availability can depend on the backend, target platform, and bundled runtime dependencies.

Example imports:

```nx
import std.io
import std.fs
import std.net
import std.json
import std.db
import std.memory
import std.crypto
```

## Nexus Paint template

The repository includes a raster-canvas application template:

```powershell
nexus new NexusPaint --template paint
cd NexusPaint
nexus build --release
nexus run
```

Consult the template and current documentation for its supported features.

## Build from source

### Requirements

- CMake 3.20 or newer
- Ninja (for the commands below)
- A C++20-capable compiler
- LLVM/Clang 18+ for the LLVM compilation path
- SDL2 development libraries for graphics/runtime features

On Windows, the CI packaging workflow builds the compiler and creates the GUI installer on a Windows runner. Building locally from source may require Visual Studio Build Tools or a compatible LLVM/MSVC toolchain and SDL2 development dependencies.

### Configure, build, and test

```powershell
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
.\build\nexus.exe --version
.\build\nexus.exe self-test
```

Linux/macOS builds use the corresponding executable path (for example, `./build/nexus --version`). See the [CMake configuration](CMakeLists.txt), [docs](docs/), and CI workflows for the currently tested build matrix.

## Repository map

- `src/` — compiler frontend, semantic analysis, code generation, driver, and CLI
- `stdlib/` — standard library modules
- `examples/` — example programs, assets, and project templates
- `tests/` — regression and runtime tests
- `docs/` — user and developer documentation
- `.github/workflows/` — CI, packaging, and documentation deployment
- `.github/project-tools/` — repository automation, Windows packaging, installer definition, and editor tooling

## Contributing

Issues, bug reports, documentation improvements, tests, and focused pull requests are welcome. Please include reproduction steps for bugs and run the relevant tests before opening a pull request.

- [Open an issue](https://github.com/fanxcz/nexus-lang/issues)
- [Pull requests](https://github.com/fanxcz/nexus-lang/pulls)
- [Contributing guide](https://github.com/fanxcz/nexus-lang/blob/main/.github/project-tools/repository-files/CONTRIBUTING.md)
- [Development roadmap](https://fanxcz.github.io/nexus-lang/development/roadmap/)

## License

NEXUS is distributed under the MIT License. See [LICENSE](LICENSE).
