# Changelog

All notable changes to NEXUS are recorded here. The project follows
[Semantic Versioning](https://semver.org/) and this file is organized in the
spirit of [Keep a Changelog](https://keepachangelog.com/).

Release notes for versions before 2.1.0 and the detailed per-change history
live in [`repository-files/CHANGELOG.md`](.github/project-tools/repository-files/CHANGELOG.md).

## Unreleased

### Fixed

- Project commands now reject a structurally malformed `nexus.toml` with a
  file-and-line diagnostic instead of silently using defaults. Truncated table
  headers, lines without `key = value`, and duplicate tables or keys fail with
  a non-zero exit code.
- The Poker packaging workflow no longer races the documentation deployment.
  It reads the installer version from `CMakeLists.txt`, retries while GitHub
  Pages publishes the file, and rejects an HTML error page served as
  `NEXUS-Setup-*.exe` or a payload whose checksum does not match.
- Added regression coverage for project paths and artifact names containing
  spaces, and for graphics/audio entry points used before initialization.

## 2.2.0 — 2026-10-28

### Added

- i64 bitwise operators `&`, `|`, `^`, `<<`, `>>` and the compound assignments
  `&=`, `|=`, `^=`, `<<=`, `>>=`. They are supported by the lexer, parser,
  semantic analyzer, formatter, LLVM code generation and the built-in x86-64
  native backend.
- Exhaustiveness checking for `match` over `bool` and enum domains. A missing
  variant is reported at compile time.
- Compile-time diagnostics for division by zero and out-of-range constant
  array indices.
- Regression tests for bitwise expressions, short-circuit evaluation,
  statement boundaries and compound assignment on fields, elements and
  dereferences.

### Changed

- The compiler is split into focused translation units: `lexer`, `parser`,
  `semantic`, LLVM and native code generation, the driver, the builtins table,
  the LSP server helpers and the REPL helpers.
- The runtime is assembled from topic-based `.inc` files under `src/runtime/`
  (`core`, `graphics`, `audio`, `io`, `data`, `strings`, `collections`, `ui`,
  `game`, `math`, `memory`, `net`, `crypto`). `runtime.cpp` is now a thin
  translation unit.
- Install rules, portable packaging and the release workflow copy the complete
  runtime tree, so an installed compiler can build with either backend.
- `CMakeLists.txt` defines shared source groups used by every target.

### Fixed

- `&&` and `||` now short-circuit correctly in both backends.
- Compound assignment works on struct fields, array elements and dereferences.
- Statements are no longer merged or split incorrectly when newlines are
  present or absent.
- `match` over a single enum payload variant is no longer mistaken for full
  coverage of all return paths.

## 2.1.1 — 2026-10-26

- Added root-level `Install-NEXUS.bat` and `Install-NEXUS.ps1` bootstrap
  scripts that download the official installer, verify its SHA-256 digest and
  launch setup only after verification.
- Added a basic single-line UTF-8 text field to the immediate-mode UI layer
  with click focus, code-point-aware Backspace and Enter/Tab completion.
- Added checked extraction helpers for `Option` and `Result`.
- Fixed enum-match return-path analysis and tightened CLI argument
  diagnostics.

## 2.1.0 — 2026-10-26

- Added the interactive REPL, project templates, formatting and linting
  commands, test support, diagnostics and VS Code language tooling.
- Expanded the standard library and runtime foundations: files and input,
  filesystem helpers, networking helpers, JSON, SQLite, SHA-256, memory
  utilities, graphics and audio.
- Added floating-point `sin`, `cos`, `tan`, `atan2`, `floor`, `ceil` and `pow`
  built-ins to the LLVM path.
- Added the native app sample, NEXUS Paint, the UI Playground and NEXUS Poker
  examples, plus the source-buildable NEXUS Studio.
