# Contributing to NEXUS

Thank you for your interest in contributing to NEXUS! This guide will help you get started.

## Code of Conduct

Please read and follow our [Code of Conduct](https://github.com/fanxcz/nexus-lang/blob/main/CODE_OF_CONDUCT.md).

## Ways to Contribute

- **Bug reports** — Use the [bug template](https://github.com/fanxcz/nexus-lang/issues/new?template=bug_report.yml)
- **Feature requests** — Use the [feature template](https://github.com/fanxcz/nexus-lang/issues/new?template=feature_request.yml)
- **Documentation** — Improve docs, fix typos, add examples
- **Code contributions** — Fix bugs, implement features, improve performance
- **Testing** — Write tests, test on different platforms
- **Community** — Help in discussions, review PRs

## Development Setup

### Prerequisites

See [Installation Guide](../getting-started/installation.md#building-from-source).

### Fork and Clone

```bash
# Fork on GitHub, then clone your fork
git clone https://github.com/YOUR_USERNAME/nexus.git
cd nexus

# Add upstream remote
git remote add upstream https://github.com/fanxcz/nexus-lang.git
```

### Build

```bash
# Configure with debug preset
cmake --preset=debug

# Build
cmake --build --preset=debug

# Run tests
ctest --test-dir build --output-on-failure
```

### Run Locally

```bash
# Build and run a test project
./build/nexus new test_game --template game
cd test_game
../build/nexus build
../build/nexus run
```

## Workflow

### 1. Create an Issue

Before starting work, create or claim an issue:
- Bug: Use bug report template
- Feature: Use feature request template
- Discuss design in the issue

### 2. Create a Branch

```bash
git checkout main
git pull upstream main
git checkout -b feat/my-feature
# or
git checkout -b fix/my-bugfix
```

Branch naming:
- `feat/` — New features
- `fix/` — Bug fixes
- `docs/` — Documentation
- `refactor/` — Code refactoring
- `test/` — Tests
- `chore/` — Maintenance

### 3. Make Changes

- Write code following [style guide](#style-guide)
- Add tests for new functionality
- Update documentation
- Run `nexus fmt` and `nexus lint`

### 4. Test

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
./build/nexus_tests --gtest_filter=LexerTest.*

# Test project creation
./build/nexus new test_proj --template app
cd test_proj && ../build/nexus build && ../build/nexus run
```

### 5. Commit

```bash
git add .
git commit -m "feat: add awesome feature

- Implement feature X
- Add tests for X
- Update docs

Fixes #123"
```

Commit message format:
- `feat:` — New feature
- `fix:` — Bug fix
- `docs:` — Documentation
- `refactor:` — Refactoring
- `test:` — Tests
- `chore:` — Maintenance
- `perf:` — Performance
- `ci:` — CI changes

### 6. Push and Create PR

```bash
git push origin feat/my-feature
```

Open PR against `fanxcz/nexus:main`.

### 7. Code Review

- Address review comments
- Keep PR updated with main
- All CI checks must pass

## Style Guide

### C++ (Compiler)

- **Standard**: C++20
- **Formatting**: `nexus fmt` (clang-format via `.clang-format`)
- **Naming**:
  - Types: `PascalCase` (`Token`, `AstNode`)
  - Functions/Variables: `snake_case` (`parse_expression`, `token_list`)
  - Constants: `SCREAMING_SNAKE_CASE` (`MAX_TOKENS`)
  - Namespaces: `lowercase` (`nexus`, `compiler`)
- **Headers**: Self-contained, `#pragma once`
- **Includes**: System → Project → Local
- **Error Handling**: Return `Result<T, Error>`, avoid exceptions

### NEXUS (Language)

- **Formatting**: `nexus fmt`
- **Naming**:
  - Types: `PascalCase` (`Vec2`, `GameState`)
  - Functions/Variables: `snake_case` (`update_player`, `max_health`)
  - Constants: `SCREAMING_SNAKE_CASE` (`DEFAULT_SPEED`)
  - Modules: `lowercase` (`game`, `render`)
- **Indentation**: 4 spaces
- **Line Length**: 100 characters max

### Documentation

- Markdown with Mermaid diagrams where helpful
- Code examples must compile
- Update relevant docs with code changes

## Testing

### Unit Tests (C++)

```cpp
// tests/test_lexer.cpp
#include <gtest/gtest.h>

TEST(LexerTest, TokenizesIntegers) {
    Lexer lexer("42");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 2); // INT(42), EOF
    EXPECT_EQ(tokens[0].type, TokenType::Integer);
    EXPECT_EQ(tokens[0].value, "42");
}
```

### Integration Tests

```cmake
# tests/CMakeLists.txt
add_test(NAME my_feature_test
    COMMAND ${NEXUS_BIN} build examples/my_feature.nx
    COMMAND ./build/my_feature
)
```

### Example Tests

Add examples to `examples/` that demonstrate the feature.

## Pull Request Checklist

- [ ] Issue referenced in PR description
- [ ] Branch up to date with main
- [ ] All CI checks pass
- [ ] `nexus fmt` applied
- [ ] `nexus lint` passes
- [ ] Tests added/updated
- [ ] Documentation updated
- [ ] CHANGELOG.md updated (for user-facing changes)
- [ ] No new warnings

## Release Process

Maintainers only:

1. Update version in `CMakeLists.txt` and `nexus.toml.example`
2. Update `CHANGELOG.md`
3. Create release branch: `release/v1.2.0`
4. Tag: `git tag v1.2.0`
5. Push tag: `git push upstream v1.2.0`
6. GitHub Actions builds and publishes release
7. Update documentation version

## Getting Help

- **Discord**: [NEXUS Community](https://discord.gg/nexus-lang)
- **GitHub Discussions**: [Q&A](https://github.com/fanxcz/nexus-lang/discussions)
- **Issues**: [Bug/Feature](https://github.com/fanxcz/nexus-lang/issues)

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](https://github.com/fanxcz/nexus-lang/blob/main/LICENSE).