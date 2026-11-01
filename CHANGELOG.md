# Changelog

All notable changes to NEXUS are recorded here. The project follows
[Semantic Versioning](https://semver.org/) and this file is organized in the
spirit of [Keep a Changelog](https://keepachangelog.com/).

Release notes for versions before 2.1.0 and the detailed per-change history
live in [`repository-files/CHANGELOG.md`](.github/project-tools/repository-files/CHANGELOG.md).

## Unreleased

### Added

- Add `std.os.config_dir(app_name)` for per-user application configuration
  directories on Windows, macOS and Linux, with `std.os.config_error()` exposing
  failure details. Unsafe app-name path components are rejected; runtime tests cover
  creation, invalid names, traversal and filesystem failure.

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

- The LSP JSON-RPC string decoder now handles all standard JSON string escapes, BMP Unicode `\uXXXX` escapes and UTF-16 surrogate pairs, converting escaped Unicode to UTF-8. A dedicated CTest covers valid and invalid escape sequences, JSON-RPC ids, positions, and watched-file capability detection.

- The LSP protocol field reader now skips quoted JSON values when resolving keys, preventing embedded `\"uri\"`/`\"line\"` text from being mistaken for protocol fields. Unit tests cover these decoy cases as well as escaped Unicode and watched-file capability parsing.

- The LSP now advertises and implements `textDocument/prepareRename`.
  The request returns a UTF-16 identifier range and placeholder only when the
  cursor resolves to a renameable symbol. Regression coverage includes invalid
  cursor positions and an astral Unicode character before the selected token.

- Added dependency-free Node.js contract tests for the VS Code extension's
  language registration, grammar, language configuration, snippets and LSP
  client launch contract. CI runs the checks alongside the existing Studio
  source/path-security checks.

- NEXUS Studio now launches the installed compiler's LSP process for the active project,
  sends the open `.nx` document lifecycle, and renders published compiler diagnostics
  as Ace annotations. Its JSON-RPC client validates UTF-8 and message framing,
  handles process shutdown, and has dependency-free Node.js tests.

- NEXUS Studio now uses the active document's LSP session for Ctrl+Space completion,
  hover information, and F12 go-to-definition. The main process exposes only explicit
  language-feature IPC handlers; definition targets are checked to remain project-local
  
- NEXUS Studio now supports multiple open document tabs with independent in-memory
  buffers and dirty state. Switching documents serializes tab operations, preserves
  unsaved text, and confirms before closing a modified tab or replacing the project.

- Added source-over alpha compositing for CPU canvases, contain-fit image import
  through SDL_image with BMP fallback, and optional PNG export through
  `IMG_SavePNG`. Canvas creation/import are capped at 16 megapixels; BMP file
  reads are bounded, and each canvas keeps at most 32 MiB of undo snapshots.
- NEXUS Paint now supports a base canvas plus three transparent raster layers,
  independent per-layer undo/redo, layer visibility, image import, and flattened
  BMP export with optional PNG export. Runtime CTests cover pixel compositing,
  image fitting, and the undo memory budget; the Paint smoke test checks the
  layered editor and keeps the project template byte-identical to the app.

- Added `scene_apply(scene)` to restore a saved scene snapshot into the live ECS
  registry. It returns the number of recreated entities, returns `-1` for an
  invalid snapshot, and explicitly invalidates previous ECS entity handles.
  The NEXUS Editor example uses this operation when loading a saved scene.

- Expanded the immediate-mode UI text field with caret movement, UTF-8 selection and
  replacement, Backspace/Delete, Escape-to-restore and Tab/Shift+Tab traversal. Added
  a stable-ID variant while retaining the rectangle-identified legacy API, plus
  runtime tests for Unicode boundary handling and byte-limited edits.

### Changed

- Moved pre-`project()` compiler selection into `cmake/NexusOptions.cmake`,
  then split dependency discovery, source groups, compiler targets, installation
  and CTest registrations into `NexusDependencies`, `NexusSources`,
  `NexusTargets`, `NexusInstall` and `NexusTests`. The root CMake file now
  orchestrates the modules while retaining CMake 3.20 and C++20.

- Split the build entry point into `NexusOptions`, `NexusDependencies`,
  `NexusSources`, `NexusTargets`, `NexusInstall` and `NexusTests` modules.
  Compiler selection remains before `project()`; dependency detection and
  target/install/test setup follow platform initialization.
- Package lockfile refresh now rejects symlinked local package directories and
  manifests, any symlink within a checksummed package tree, and paths that traverse out
  of `vendor/`; canonical package paths are confined before manifests are read or trees
  are hashed.
- The NEXUS Studio desktop packaging matrix now also runs on pull requests
  that change Studio sources or the workflow, building Windows, Linux and macOS
  packages and uploading the generated artifacts for inspection.

- Refreshed the documentation landing page and shared site styling, added an
  ecosystem integration map, and clarified which Studio, Paint, backend and
  distribution capabilities are currently implemented or constrained.

- Improved Studio's narrow-window layout, keyboard focus visibility and
  accessible status/output announcements. The child-process slot remains
  occupied until the process close event, preventing overlapping build/terminal
  processes during shutdown.

- Version numbers now come from one place: `scripts/nexus_version.py` reads the
  compiler version from `CMakeLists.txt` and the extension version from
  `package.json`. The Pages and release workflows, download validators,
  installer output name and bootstrap version pins use or are checked against
  those values by the new `nexus_version_consistency` CTest. The installer no
  longer describes itself as "NEXUS 2.1".

### Fixed

- Scene snapshot IO validates finite numeric fields, rejects truncated, oversized and
  trailing records, preserves float round-trip precision, and reports failed writes.
  Scene application builds a replacement entity map before swapping it into the live ECS,
  so allocation failures do not leave a partially cleared world. Regression tests cover
  malformed snapshots and save failures.

- The Windows-only Task Manager now accepts only plain executable-name tokens for
  `tasklist`, `taskkill` and `start` calls, and enforces the full unsigned 32-bit PID
  range (1 through 4,294,967,295). A CTest extracts and executes the actual validators
  against shell metacharacters, paths, malformed PIDs and numeric boundary values.
  NEXUS Showcase also exits cleanly when graphics initialization fails.

- NEXUS Paint now checkpoints the canvas before **New**, reports failed BMP save/load operations
  instead of leaving a stale success status, and reports a failed final save at shutdown.
  The project template is kept byte-for-byte synchronized with the standalone example and
  the Paint smoke test enforces that contract.

- NEXUS Editor no longer destroys its active scene handle when loading a file.
  It applies a successfully loaded snapshot to the live ECS registry, keeps the
  new snapshot handle active, and reports save/load/apply failures in the UI.

- Cross-platform native-backend smoke tests now distinguish unsupported language features
  on Windows/Linux from the target-level absence of a built-in native emitter on macOS.
  Runtime memory range-test failures identify the exact range case and expected/actual result.

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
