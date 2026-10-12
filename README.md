# NEXUS 1.1.0

[![CI](https://github.com/fanxcz/nexus/actions/workflows/build.yml/badge.svg)](https://github.com/fanxcz/nexus/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-1.1.0-blue.svg)](https://github.com/fanxcz/nexus/releases)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg)](https://github.com/fanxcz/nexus)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![LLVM](https://img.shields.io/badge/LLVM-18%2B-orange.svg)](https://llvm.org/)
[![Documentation](https://img.shields.io/badge/docs-online-brightgreen.svg)](https://fanxcz.github.io/nexus/)

NEXUS is a native programming language and application/game runtime. The compiler is developed in C++20 and emits LLVM IR/native binaries through Clang/LLVM. The repository has six top-level directories: `.github/`, `docs/`, `examples/`, `src/`, `stdlib/`, and `tests/`. Starter templates live in `examples/templates/`; build, installer, and packaging helpers are grouped under `.github/project-tools/`.

**Website and downloads:** https://fanxcz.github.io/nexus/ · [Windows installer (.exe)](https://fanxcz.github.io/nexus/assets/downloads/NEXUS-Setup-1.1.0.exe) · [Portable ZIP](https://fanxcz.github.io/nexus/assets/downloads/NEXUS_Windows_Portable.zip) · [VS Code extension (.vsix)](https://fanxcz.github.io/nexus/assets/downloads/NEXUS-Language-1.1.0.vsix)

## What is working

- Native compiler pipeline: lexer -> parser -> semantic checking -> LLVM IR -> native binary.
- Project workflow: `new`, `init`, `build`, `run`, `check`, `clean`, `info`, `assets`, `doctor`, and `self-test`.
- Tooling: `fmt` (including non-mutating `--check`), `lint`, `doc`, `bench`, `editor`, and `lsp`.
- Local package workflow: `nexus pkg init/add/remove/list/update/build/publish`.
- Runtime: memory allocation, files, environment, input, HTTP GET facade, lightweight JSON field extraction, dynamic SQLite runtime loading, SHA-256 helper, SDL2/OpenGL 2D/3D, audio, UI, ECS, scenes, particles and physics.
- Targets: Linux x86_64 is the primary development target. Windows/macOS CI jobs are included; native release validation remains required.

## Create a project

```bash
nexus new MyGame --template game
cd MyGame
nexus build
nexus run
```

## Standard modules

```nx
import std.io
import std.fs
import std.net
import std.json
import std.db
import std.memory
import std.crypto
```

## Tooling

```bash
nexus fmt
nexus fmt --check src/main.nx
nexus lint
nexus doc
nexus bench --runs 5
nexus test
nexus self-test
nexus doctor --json
nexus editor
nexus lsp
```

The repository also includes [VS Code language-extension source](https://github.com/fanxcz/nexus/tree/main/.github/project-tools/vscode-extension) for `.nx` syntax highlighting, snippets, compiler diagnostics, completion and limited declaration navigation. Installable VSIX builds are published on the [downloads page](https://fanxcz.github.io/nexus/downloads/). Package/install instructions: [VS Code extension guide](https://fanxcz.github.io/nexus/tooling/vscode-extension/).

## Explain backend selection

Use <code>nexus explain-backend</code> to see which backend <code>auto</code> will use for a source file and why. The report identifies whether the program fits the built-in x86-64 subset or needs the LLVM fallback.

<pre><code>nexus explain-backend src/main.nx
nexus explain-backend src/main.nx --target x86_64-linux
nexus doctor</code></pre>

`nexus doctor --json` provides machine-readable environment status, and `nexus self-test` compiles and runs a tiny program, preferring the built-in native backend where supported. The formatter has a non-mutating `nexus fmt --check` mode for CI. See the [CLI command reference](https://fanxcz.github.io/nexus/tooling/commands/) and the [development roadmap](https://fanxcz.github.io/nexus/development/roadmap/) for the current feature boundaries and next priorities.

The CLI version is generated from the CMake project version, so the compiled version and release metadata share one source of truth.

## Package workflow

Local packages can be vendored into `vendor/` and locked in `nexus.lock`:

```bash
nexus pkg init
nexus pkg add ../my-library
nexus pkg list
nexus pkg build
nexus pkg publish
```

## Important limits in 1.1.0

The runtime/tooling foundation is production-oriented, but some advanced language features remain experimental or are still being designed: full generic monomorphization, trait dispatch, closures/async lowering, a full visual scene editor, and a complete JSON AST/HTTP server stack. They are isolated behind stable interfaces so they can be added without redesigning the project format.

## Builtin native backend (no toolchain required)

Subset programs (`i64`/`bool`/`string`, functions, control flow, `print`/`input`, string concatenation/comparison) compile straight to x86-64 Windows (PE) or Linux (ELF) executables without LLVM, Clang, or any SDK:

```bash
nexus build app.nx -o app --backend native
nexus run app.nx --backend native   # default `auto` picks native when possible
```

`nexus export` produces a release binary plus a distributable directory (and a zip archive when `cmake` is available). Details and subset limits: `docs/tooling/native-backend.md`.

## Nexus Paint

NEXUS includes a native editable raster canvas example and project template:

```bash
nexus new NexusPaint --template paint
cd NexusPaint
nexus build --release
nexus run
```

The Paint template supports a 1024x720 canvas, pencil, eraser, line, rectangle, flood fill, color palette, brush size, undo/redo, and BMP save/load.

## Graphics backend (1.1.0)

Nexus 2D applications use the SDL2 renderer by default. This keeps windowed, maximized and fullscreen layouts independent from an OpenGL compatibility profile. OpenGL is enabled lazily when a 3D frame is requested.

Useful window controls:

```nx
gfx_fullscreen(true)
gfx_fullscreen(false)
gfx_maximize()
gfx_restore()
print(gfx_backend())
```

Applications can use a stable virtual UI space with `gfx_set_ui_resolution()`; mouse coordinates are converted into the same logical coordinate system.

## Downloads

- **Windows GUI installer:** [NEXUS-Setup-1.1.0.exe](https://fanxcz.github.io/nexus/assets/downloads/NEXUS-Setup-1.1.0.exe). Provides checkbox options for a Desktop shortcut, Start menu shortcuts, adding `nexus` to the user `PATH`, and a `.nx` context-menu command. Installs into the current user's profile without administrator privileges.
- **Self-contained Windows portable ZIP:** [Download](https://fanxcz.github.io/nexus/assets/downloads/NEXUS_Windows_Portable.zip). Includes the NEXUS compiler, bundled Zig-based C++ compiler/linker, Windows target libraries, SDL2 runtime, standard library, templates, and examples. Building `.nx` applications does not require installing LLVM or Visual Studio Build Tools.
- **Linux x86-64 and macOS archives:** [GitHub Releases](https://github.com/fanxcz/nexus/releases). The tag-triggered workflow packages the compiler, standard library, templates, and pinned Zig-based C++ toolchain for the LLVM fallback, with separate Apple Silicon and Intel archives. These assets appear after the first version tag is published.
- **Documentation and website:** [NEXUS on GitHub Pages](https://fanxcz.github.io/nexus/)
- **Full source:** this repository; generated build output and editor caches are excluded.

## Documentation

- [Language Reference](https://fanxcz.github.io/nexus/language/)
- [Runtime API](https://fanxcz.github.io/nexus/runtime/)
- [Standard Library](https://fanxcz.github.io/nexus/std/)
- [Tooling](https://fanxcz.github.io/nexus/tooling/)
- [Examples](https://fanxcz.github.io/nexus/examples/)

## Building from Source

### Prerequisites

- CMake 3.20+
- Ninja
- Clang/LLVM 18+ (or MSVC 19.40+ on Windows)
- SDL2 development libraries

### Build

```bash
# Configure
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON

# Build
cmake --build build

# Test
ctest --test-dir build --output-on-failure

# Run
./build/nexus --version
```

### Using CMake Presets (VS Code / CLion)

```bash
# List available presets
cmake --list-presets

# Configure with a preset
cmake --preset=release

# Build with a preset
cmake --build --preset=release
```

Available presets: `default`, `debug`, `release`, `relwithdebinfo`, `msvc`, `msvc-debug`, `coverage`, `sanitize-address`.

## Publishing the site

The documentation site and Windows portable package are built and published by [GitHub Actions](https://github.com/fanxcz/nexus/actions/workflows/pages.yml).

## Contributing

See [CONTRIBUTING.md](https://github.com/fanxcz/nexus/blob/main/.github/project-tools/repository-files/CONTRIBUTING.md) for guidelines.

## License

MIT License - see [LICENSE](https://github.com/fanxcz/nexus/blob/main/LICENSE) for details.

## Community

- [GitHub Issues](https://github.com/fanxcz/nexus/issues) - Bug reports, feature requests, questions
- [GitHub Discussions](https://github.com/fanxcz/nexus/discussions) - General discussion