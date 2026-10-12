# CMake integration

CMake builds the NEXUS compiler and its test programs. The project's `CMakeLists.txt` sets the minimum supported CMake version to 3.20 and requests C++20. Ninja is used by the repository presets and CI, but another CMake generator can be selected explicitly.

## Requirements

- CMake 3.20 or newer.
- A C++20-capable compiler and a compatible build tool.
- Ninja when using the checked-in presets or the commands below.
- SDL2 development libraries are optional for compiling the compiler; without them, graphics/audio runtime features may be unavailable.
- A working Clang/C++ toolchain is needed for NEXUS programs that use the LLVM fallback. Supported built-in-native-subset programs do not need LLVM to be installed.

## Configure, build, and test

From the repository root:

```sh
cmake --preset=release
cmake --build --preset=release
ctest --test-dir build --output-on-failure
./build/nexus --version
./build/nexus self-test
```

For a debug build, replace `release` with `debug`. Or use explicit CMake options:

```sh
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, the resulting compiler is normally `build/nexus.exe`; on Linux and macOS it is `build/nexus`.

## Available presets

The checked-in `CMakePresets.json` defines `default`, `debug`, `release`, and `relwithdebinfo`. Windows-only presets include `msvc` and `vcpkg`. Inspect their actual settings with:

```sh
cmake --list-presets
```

The `vcpkg` preset expects `VCPKG_ROOT` to point to a configured vcpkg checkout and uses its CMake toolchain file.

## Run focused tests

To inspect failures, use:

```sh
ctest --test-dir build --output-on-failure
```

The test set covers the compiler, diagnostics, project scaffolding, native backend, runtime memory, language-server and VS Code-extension smoke checks. Some platform-specific tests are only registered on supported operating systems.
