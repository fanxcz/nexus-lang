# Getting Started

Welcome to NEXUS! This section will help you install NEXUS and create your first project.

## In This Section

| Guide | Description |
|-------|-------------|
| [Installation](installation.md) | Install NEXUS on your platform |
| [Quick Start](quickstart.md) | Create your first project in 5 minutes |
| [Project Structure](project-structure.md) | Understand the project layout |

## Prerequisites

- **OS**: Linux (x86_64), Windows (x86_64), macOS (x86_64/arm64)
- **Compiler**: Clang 18+, GCC 13+, or MSVC 19.40+
- **Build Tools**: CMake 3.20+, Ninja
- **Dependencies**: SDL2 development libraries

## Quick Install

```bash
# Linux (Ubuntu/Debian)
sudo apt-get install cmake ninja-build clang-18 lld-18 libsdl2-dev

# Build from source
git clone https://github.com/fanxcz/nexus.git
cd nexus
cmake --preset=release
cmake --build --preset=release
sudo cmake --install build
```

## Verify Installation

```bash
nexus --version
# nexus 1.1.0
```

## Next Steps

1. Follow the [Quick Start](quickstart.md) to create your first project
2. Learn the [Language Basics](../language/overview.md)
3. Explore the [Standard Library](../std/overview.md)
4. Check out [Examples](../examples/overview.md)