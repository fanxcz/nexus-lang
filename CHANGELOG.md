# Changelog

All notable changes to NEXUS are recorded here. The project follows
[Semantic Versioning](https://semver.org/) and this file is organized in the
spirit of [Keep a Changelog](https://keepachangelog.com/).

Release notes for versions before 2.1.0 and the detailed per-change history
live in [`repository-files/CHANGELOG.md`](.github/project-tools/repository-files/CHANGELOG.md).

## Unreleased

### Added

- Added explicit exclusive mutable slices using `mut []T` and `mut array[start..end]`.
  Mutable slices support indexed writes (including compound assignment), function
  parameters and `len` on the LLVM backend. The semantic analyzer rejects immutable
  owners, overlapping active borrows, accesses to an owner while it is borrowed,
  shared/mutable borrow conflicts, and returning a mutable slice. Native codegen
  continues to reject slice types and automatic backend selection uses LLVM.

- Fixed-width numeric scalar types: signed `i8`/`i16`/`i32`, unsigned
  `u8`/`u16`/`u32`/`u64`, and `f32`, alongside the existing `i64`/`f64`.
  LLVM lowering preserves integer width and signedness for arithmetic,
  comparisons, division, shifts, struct/array fields, and scalar enum payloads.
  Numeric literals adopt an expected numeric type when representable; ordinary
  variable-to-variable conversions remain explicit. The built-in native backend
  continues to reject these types and `--backend auto` uses LLVM for them.

- Added executable slice support on the LLVM backend. `[]T` values use a
  `{ ptr, i64 }` representation, support half-open slicing of named arrays and
  existing slices, dynamic bounds checks, indexed reads, polymorphic `len` for
  strings/arrays/slices, and by-value function parameters/returns. Borrow-origin
  analysis rejects returning a view
  into a local array; slices remain unsupported by the native backend and in
  struct fields or arrays of slices.

- `nexus.toml` now accepts arrays, including multi-line and nested arrays with
  comments and trailing commas, and repeated `[[array-of-tables]]` headers.
  Project and package commands share one parser, so both accept and reject the
  same files. Arrays are validated structurally; scalar fields still read only
  scalar values.

### Changed

- Version numbers now come from one place: `scripts/nexus_version.py` reads the
  compiler version from `CMakeLists.txt` and the extension version from
  `package.json`. The Pages and release workflows, download validators,
  installer output name and bootstrap version pins use or are checked against
  those values by the new `nexus_version_consistency` CTest. The installer no
  longer describes itself as "NEXUS 2.1".

### Fixed

- `nexus_gfx_audio_available()` now reports whether an audio device is open, not merely whether the SDL shared library can be loaded. Headless systems no longer report audio as available before a device exists.
- Passing a directory as a source file now reports `cannot open <path>: is a
  directory` on every platform. POSIX hosts previously leaked libstdc++'s
  internal `basic_filebuf::underflow` message because opening a directory
  succeeds and only the read fails.
- Project commands now reject a structurally malformed `nexus.toml` with a
  file-and-line diagnostic instead of silently using defaults. Truncated table
  headers, lines without `key = value`, and duplicate tables or keys fail with
  a non-zero exit code.
- The Windows packaging workflow no longer races the documentation deployment.
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
- Added the native app sample, NEXUS Paint, the UI Playground and the
  source-buildable NEXUS Studio.
