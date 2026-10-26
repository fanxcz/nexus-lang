# NEXUS

![NEXUS](docs/assets/images/nexus-logo.svg)

**NEXUS is an open-source programming language and native runtime for applications, tools, and games.** Its compiler is written in C++20. It includes a built-in x86-64 native backend for a documented subset of the language and an LLVM-based compilation path for broader support.

[![CI](https://github.com/fanxcz/nexus-lang/actions/workflows/build.yml/badge.svg)](https://github.com/fanxcz/nexus-lang/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/fanxcz/nexus-lang/blob/main/LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

**Repository:** https://github.com/fanxcz/nexus-lang  
**Documentation and downloads:** https://fanxcz.github.io/nexus-lang/  
**Windows installer:** download `NEXUS-Setup-2.1.0.exe` from the [project downloads page](https://fanxcz.github.io/nexus-lang/downloads/) or from a successful [GitHub Actions Pages run](https://github.com/fanxcz/nexus-lang/actions/workflows/pages.yml).

> The website URL above is the GitHub Pages project URL for this repository. Do not use the old `fanxcz.github.io/nexus/` address; that belongs to a different repository/site.

## What's new in 2.1.0

- NEXUS Poker 2.1.0 improves five-card hand ranking and kicker tie-breaks, including suit-aware straight-flush detection.
- Poker now renders distinct illustrated opponents, a stable spade cursor, smoothly orbiting turn markers and a compact hand-strength meter.
- The poker example has a persistent cosmetic table-skin shop; its purchases use earned chips and do not change game rules or bot strength.
- The LLVM compilation path adds floating-point `sin`, `cos`, `tan`, `atan2`, `floor`, `ceil` and `pow` built-ins for games and simulation.
- Graphics runtime cursor visibility is idempotent: repeated requests for the same visibility state are skipped, avoiding redundant SDL cursor-state changes.
- Release metadata, installer names and the VS Code language extension are aligned on version 2.1.0.

## Core language features introduced in 2.0

- Growable lists (`ilist_*` / `slist_*`), `capture_output`, string builtins (`contains`, `find`, `trim`, `upper`, `lower`, `slice`, `split_*`)
- Smart concatenation: `"code=" + 42` works without `str_i64`
- Unicode string escapes: `"Hello \u{1F642}"` decode to UTF-8 during lexing
- Python-style REPL bindings (`x = 4` creates the variable)
- Default system fonts, text measurement and font styles in the graphics API
- FFI confirmed against the Windows API — see `examples/cheat_engine.nx`, a working
  Cheat Engine style memory scanner written in NEXUS (`OpenProcess`, `VirtualQueryEx`,
  `Read/WriteProcessMemory`, `VirtualProtectEx`)
- A self-hosting toolchain in `selfhost/`: lexer, parser and type checker written in
  NEXUS itself, all covered by ctest
- VS Code extension: syntax highlighting, snippets and LSP integration
  (`.github/project-tools/vscode-extension`)

## Current status

NEXUS is under active development. The command-line compiler, built-in native backend, project templates, standard library, runtime foundations, formatter/linter, diagnostics, and tests are present in this repository. Some advanced language and runtime features remain experimental; consult the documentation and tests before relying on a feature in production.

The built-in native backend supports a defined subset (including integer, boolean and string values, functions, control flow, printing/input, and selected string operations) and can emit x86-64 Windows PE or Linux ELF executables without a separately installed LLVM/Clang toolchain. Programs outside that subset may require the LLVM compilation path and its toolchain.

## Install on Windows 11

1. Open the [NEXUS downloads page](https://fanxcz.github.io/nexus-lang/downloads/) or the [Pages workflow runs](https://github.com/fanxcz/nexus-lang/actions/workflows/pages.yml).
2. Download `NEXUS-Setup-2.1.0.exe` only from an official project artifact.
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

## Interactive terminal

Run NEXUS without arguments to open its interactive shell, or start it explicitly with
nexus repl. Enter an expression to evaluate it immediately:

```text
nexus> 2 + 2
4
nexus> let mut answer = 21
nexus> answer * 2
42
nexus> answer = answer + 1
nexus> answer
22
```

Use `:help` for examples, `:vars` to list saved bindings, `:clear` to reset the
session, and `:quit` to exit. The shell uses the NEXUS compiler and language syntax;
it is an interactive evaluator, not a Python interpreter.

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
nexus repl
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

## VS Code extension

Syntax highlighting, snippets and LSP integration for `.nx` files.

Install without a marketplace account:

1. Download [`NEXUS-Language-2.1.0.vsix`](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Language-2.1.0.vsix) from the [NEXUS downloads page](https://fanxcz.github.io/nexus-lang/downloads/).
2. VS Code → Extensions (`Ctrl+Shift+X`) → `...` → **Install from VSIX...** → select the file.

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

## Downloadable applications

**NEXUS Studio** is a source-buildable desktop editor with a project explorer, `.nx` editor, compiler selection, and Build/Run controls. The prebuilt 89 MB installer has been removed from the source tree to keep repository downloads smaller; see the source build instructions. Install NEXUS separately, then select `nexus.cmd` in Studio.

- [NEXUS Studio source and build instructions](https://github.com/fanxcz/nexus-lang/tree/main/.github/project-tools/nexus-studio)

**NEXUS Paint (Windows x64)** is a native raster drawing app written in NEXUS. The Paint setup EXE installs the app and all runtime DLLs. The portable ZIP contains the runnable app EXE, runtime DLLs and editable NEXUS source. Download both from the official site:

- [Download NEXUS Paint portable ZIP](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.zip)
- [Download NEXUS Paint installer EXE (installs the runtime DLLs)](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.exe)
- [Verify Paint installer SHA-256](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.exe.sha256.txt)
- [Verify Paint ZIP SHA-256](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.zip.sha256.txt)
- [View Paint source](https://github.com/fanxcz/nexus-lang/blob/main/examples/nexus_paint.nx)
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

Linux/macOS builds use the corresponding executable path (for example, `./build/nexus --version`). See the [CMake configuration](https://github.com/fanxcz/nexus-lang/blob/main/CMakeLists.txt), [docs](https://github.com/fanxcz/nexus-lang/tree/main/docs), and CI workflows for the currently tested build matrix.

## Repository map

- `src/` — compiler frontend, semantic analysis, code generation, driver, and CLI
- `stdlib/` — standard library modules
- `examples/` — example programs, assets, and project templates
- `tests/` — regression and runtime tests
- `docs/` — user and developer documentation
- [Platform support matrix](https://github.com/fanxcz/nexus-lang/blob/main/docs/development/platform-support.md) — tested hosts, native backend targets, and runtime limitations
- `.github/workflows/` — CI, packaging, and documentation deployment
- `.github/project-tools/` — repository automation, Windows packaging, installer definition, and editor tooling

## Contributing

Issues, bug reports, documentation improvements, tests, and focused pull requests are welcome. Please include reproduction steps for bugs and run the relevant tests before opening a pull request.

- [Open an issue](https://github.com/fanxcz/nexus-lang/issues)
- [Pull requests](https://github.com/fanxcz/nexus-lang/pulls)
- [Contributing guide](https://github.com/fanxcz/nexus-lang/blob/main/CONTRIBUTING.md)
- [Development roadmap](https://fanxcz.github.io/nexus-lang/development/roadmap/)

## License

NEXUS is distributed under the MIT License. See [LICENSE](https://github.com/fanxcz/nexus-lang/blob/main/LICENSE).
