# Troubleshooting

This page covers common source-build and command-line failures. Start with `nexus doctor` and `nexus --version`; include their output (with private paths or usernames removed) when requesting help.

## `nexus` is not recognized

- Open a new terminal after installation so PATH changes are loaded.
- On Windows, try the installed launcher directly: `& "$env:LOCALAPPDATA\Programs\NEXUS\nexus.cmd" --version`.
- On Linux/macOS, inspect `command -v nexus`, the install directory and the shell's PATH.
- From a source build, run the binary explicitly: `./build/nexus --version` (Windows: `build\nexus.exe --version`).

## CMake configure fails

The project requires CMake 3.20 or newer, a C++20 compiler and (for the documented preset workflow) Ninja.

```sh
cmake --version
c++ --version
ninja --version
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON
```

If a previous build directory was configured with a different compiler/generator, remove that build directory before reconfiguring. CMake caches compiler selection; changing `CMAKE_CXX_COMPILER` in place is not supported.

## SDL2 is missing

SDL2 is optional at configure time. CMake emits a warning and the compiler build can continue, but graphics/audio runtime integration that depends on SDL2 may not work.

- Ubuntu/Debian: `sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev`
- Arch: `sudo pacman -S sdl2 sdl2_image sdl2_mixer sdl2_ttf`
- Fedora: `sudo dnf install SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel`
- macOS: `brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf`
- Windows: use a vcpkg toolchain with `sdl2 sdl2-image sdl2-mixer sdl2-ttf`.

Re-run CMake after installing development packages. Runtime availability still depends on the specific optional library and host.

## LLVM backend cannot find a compiler

The builtin native backend only covers its documented x86-64 Windows/Linux subset. Programs outside it may need the LLVM path and a compatible clang++-style compiler.

```sh
clang++ --version
nexus doctor
NEXUS_CXX=/absolute/path/to/clang++ nexus build app.nx --backend llvm
```

On PowerShell, set `$env:NEXUS_CXX = "C:\path\to\clang++.exe"` for the current shell. A plain GCC driver may not accept the textual LLVM IR emitted by NEXUS; use a compatible Clang/Zig-backed driver as documented by the release package.

## A feature is rejected by the native backend

Run `nexus explain-backend file.nx` and inspect the unsupported construct. Use `--backend llvm` when the feature is supported there and the toolchain is available. Do not assume that a construct accepted by the parser is supported by both code generators.

## Tests fail or a source file cannot be opened

- Run `ctest --test-dir build --output-on-failure` from the repository root.
- Pass the actual `.nx` file to `nexus check`; directories are not source files.
- For import cycles, inspect the reported import chain and make the dependency graph acyclic.
- Check relative paths from the process working directory.
- Preserve the exact compiler version, host OS, backend, command and first diagnostic when reporting a bug.

For usage questions, use [GitHub Discussions](https://github.com/fanxcz/nexus-lang/discussions). Reserve Issues for reproducible bugs and concrete work items.
