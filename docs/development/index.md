# Development

Guides for contributing to and developing NEXUS.

## In This Section

| Topic | Description |
|-------|-------------|
| [Contributing](contributing.md) | Contribution guide |
| [Building from Source](building.md) | Build instructions |
| [Testing](testing.md) | Running tests |
| [Debugging](debugging.md) | Debugging tips |

## Quick Start

```bash
# Clone and build
git clone https://github.com/fanxcz/nexus-lang-lang.git
cd nexus
cmake --preset=debug
cmake --build --preset=debug

# Run tests
ctest --test-dir build --output-on-failure
```