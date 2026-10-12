# Building from Source

Complete guide to building NEXUS compiler and runtime from source.

## Prerequisites

| Tool | Minimum | Recommended | Install |
|------|---------|-------------|---------|
| CMake | 3.20 | 3.25+ | `apt install cmake` / `choco install cmake` / `brew install cmake` |
| Ninja | 1.10 | 1.11+ | `apt install ninja-build` / `choco install ninja` / `brew install ninja` |
| Clang/LLVM | 18 | 18+ | `apt install clang-18 lld-18` / `choco install llvm` / `brew install llvm` |
| SDL2 dev | 2.28 | 2.30+ | `apt install libsdl2-dev` / `vcpkg install sdl2` / `brew install sdl2` |
| C++20 compiler | Clang 18 / GCC 11 / MSVC 19.30 | Clang 18 / GCC 13 / MSVC 19.40 | See above |

## Quick Build
**CMake version note:** the project minimum is CMake 3.20, but the checked-in `CMakePresets.json` uses schema version 6 and therefore requires CMake 3.25+ for `--preset` commands. With CMake 3.20–3.24, configure with `cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON`, then use `cmake --build build` and `ctest --test-dir build --output-on-failure`.


```bash
# Clone
git clone https://github.com/fanxcz/nexus-lang.git
cd nexus

# Configure (uses CMakePresets.json)
cmake --preset=release

# Build
cmake --build --preset=release

# Test
ctest --test-dir build --output-on-failure

# Run
./build/nexus --version
```

## Platform-Specific Instructions

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update && sudo apt-get install -y \
    cmake ninja-build clang-18 lld-18 \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev

# Build
cmake --preset=release
cmake --build --preset=release
```

### Linux (Arch)

```bash
sudo pacman -S cmake ninja clang lld sdl2 sdl2_image sdl2_mixer sdl2_ttf
cmake --preset=release
cmake --build --preset=release
```

### Linux (Fedora)

```bash
sudo dnf install cmake ninja-build clang lld SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel
cmake --preset=release
cmake --build --preset=release
```

### Windows (PowerShell)

```powershell
# Option 1: Chocolatey
choco install cmake ninja llvm sdl2

# Option 2: vcpkg (recommended for SDL2)
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf --triplet x64-windows

# Build with vcpkg
cmake --preset=vcpkg
cmake --build --preset=vcpkg

# Or manual with LLVM path
cmake -S . -B build -G Ninja `
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" `
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" `
  -DCMAKE_LINKER="C:/Program Files/LLVM/bin/lld.exe"
```

### macOS

```bash
brew install cmake ninja llvm sdl2 sdl2_image sdl2_mixer sdl2_ttf

# LLVM is keg-only, link or use full path
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
# or on Intel Mac:
export PATH="/usr/local/opt/llvm/bin:$PATH"

cmake --preset=release
cmake --build --preset=release
```

## Build Presets

| Preset | Description | Use Case |
|--------|-------------|----------|
| `default` | Auto-detect Clang, Release | Daily development |
| `debug` | Debug symbols, no optimization | Debugging |
| `release` | Optimized, no debug info | Production |
| `relwithdebinfo` | Optimized + debug info | Profiling |
| `msvc` | Windows with clang-cl | Windows dev |
| `vcpkg` | Windows with vcpkg toolchain | Windows + SDL2 |

List all: `cmake --list-presets`

## Custom Compiler

Create `CMakeUserPresets.json` (not tracked):

```json
{
  "version": 6,
  "configurePresets": [{
    "name": "my-clang",
    "inherits": "default",
    "cacheVariables": {
      "NEXUS_C_COMPILER": "/usr/bin/clang-18",
      "NEXUS_CXX_COMPILER": "/usr/bin/clang++-18",
      "NEXUS_LINKER": "/usr/bin/lld"
    }
  }]
}
```

Or pass directly:
```bash
cmake -S . -B build -G Ninja \
  -DNEXUS_C_COMPILER=clang-18 \
  -DNEXUS_CXX_COMPILER=clang++-18 \
  -DNEXUS_LINKER=lld
```

## Faster Linking

```bash
# Install mold (fastest linker)
sudo apt-get install mold
# or
cargo install mold

# CMake will auto-detect and use it
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `NEXUS_BUILD_TESTS` | ON | Build test executable |
| `NEXUS_USE_CLANG` | ON (Windows) | Auto-detect LLVM on Windows |
| `CMAKE_BUILD_TYPE` | Release | Debug/Release/RelWithDebInfo/MinSizeRel |

## Troubleshooting

### "Could NOT find SDL2"

```bash
# Linux
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev

# Windows (vcpkg)
.\vcpkg\vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf --triplet x64-windows
cmake --preset=vcpkg

# macOS
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

### "Clang not found" on Windows

1. Install LLVM: `choco install llvm`
2. Add to PATH: `C:\Program Files\LLVM\bin`
3. Or use `CMakeUserPresets.json` with explicit paths

### "C++20 not supported"

Upgrade compiler:
- Clang ≥ 18
- GCC ≥ 11
- MSVC ≥ 19.30 (VS 2022 17.3+)

### Linker errors (undefined SDL2 symbols)

```bash
# Ensure SDL2 linked
cmake -S . -B build -G Ninja -DSDL2_DIR=/path/to/sdl2/cmake
```

### Out of memory during build

```bash
# Reduce parallel jobs
cmake --build --preset=release -j4

# Or use mold linker (uses less memory)
```

## Development Workflow

```bash
# 1. Make changes
# 2. Incremental build (fast)
cmake --build --preset=debug

# 3. Run tests
ctest --test-dir build --output-on-failure

# 4. Test manually
./build/nexus new test_proj --template app
cd test_proj && ../build/nexus build && ../build/nexus run

# 5. Format code
./build/nexus fmt

# 6. Lint
./build/nexus lint
```

## Install System-Wide

```bash
# Build release
cmake --preset=release
cmake --build --preset=release

# Install
sudo cmake --install build

# Verify
nexus --version
```

## Docker Build

```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    cmake ninja-build clang-18 lld-18 \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
WORKDIR /nexus
COPY . .
RUN cmake --preset=release && cmake --build --preset=release
RUN ctest --test-dir build --output-on-failure
```

## CI/CD (GitHub Actions)

See `.github/workflows/build.yml` - builds on Linux, Windows, macOS automatically.