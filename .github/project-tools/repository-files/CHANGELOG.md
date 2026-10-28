# Changelog

## Unreleased

- Accept arrays (multi-line, nested, with comments and trailing commas) and repeated `[[array-of-tables]]` headers in `nexus.toml`. Project and package commands now share one manifest parser, so a file accepted by one is accepted by the other; arrays are validated structurally and scalar fields read only scalar values.
- Centralize version numbers in `scripts/nexus_version.py`: Pages/release workflows, download validators, the installer output name and bootstrap pins now read or are verified against `CMakeLists.txt` and the VS Code extension `package.json`. `ctest -R nexus_version_consistency` fails when a pin, artifact name or installer description goes stale, and the installer no longer claims to be "NEXUS 2.1".
- Reject structurally malformed `nexus.toml` files (truncated table headers, lines without `key = value`, duplicate tables or keys) with a file-and-line diagnostic instead of silently falling back to defaults.
- Report a directory passed as a source file as `cannot open <path>: is a directory` on every platform instead of leaking libstdc++'s `basic_filebuf::underflow` message.
- Make the Poker packaging workflow read the installer version from `CMakeLists.txt`, retry while GitHub Pages publishes it, and refuse an HTML error page or a payload with a mismatched checksum.
- Added regression tests for project paths and artifact names containing spaces, and for calling graphics/audio APIs before initialization.

## NEXUS 2.2.0 — bitwise operators, exhaustive match and a focused source tree (2026-10-28)

- Added i64 bitwise operators `&`, `|`, `^`, `<<`, `>>` and the compound assignments `&=`, `|=`, `^=`, `<<=`, `>>=`. The lexer, parser, semantic analyzer and formatter handle them, and both the LLVM path and the built-in x86-64 native backend emit identical results.
- Made `match` exhaustiveness a semantic requirement over `bool` and enum domains; a missing variant now fails compilation instead of falling through.
- Fixed `&&`/`||` short-circuit evaluation, compound assignment on struct fields, array elements and dereferences, newline-dependent statement boundaries, and added compile-time division-by-zero and constant index diagnostics.
- Split the compiler into focused translation units: `lexer`, `parser`, `semantic`, LLVM code generation, native code generation, the driver, the builtins signature table, LSP server helpers (`protocol`, `text`, `uri`, `diagnostics`) and REPL helpers all now live in dedicated files with internal headers.
- Split the runtime into topic directories (`core`, `graphics`, `audio`, `io`, `data`, `strings`, `collections`, `ui`, `game`, `math`, `memory`, `net`, `crypto`) assembled by a thin `runtime.cpp`; install rules, portable packaging and the release workflow now ship the complete tree.
- Bumped the compiler, installer, website download links and bootstrap scripts to 2.2.0. Added `CMake` source groups so the split files build and install consistently across targets.
- Added regression coverage for bitwise expressions, short-circuit evaluation, statement lines and compound assignment on fields, elements and dereferences.

## NEXUS 2.1.1 — installer and UI improvements (2026-10-26)

- Added root-level `Install-NEXUS.bat` and `Install-NEXUS.ps1` bootstrap scripts. They download the official Windows installer, validate its SHA-256 digest and launch setup only after verification.
- Bumped the compiler/setup package to 2.1.1; the Pages workflow builds and publishes `NEXUS-Setup-2.1.1.exe` with a matching checksum.
- Added a basic single-line UTF-8 UI text field with click focus, code-point-aware Backspace, bounded UTF-8 input, and Enter/Tab to finish editing. The field currently uses stable layout bounds rather than explicit widget IDs; Escape cancellation, caret/selection editing, and multi-field keyboard traversal remain future work.
- Improved checked `Option`/`Result` helpers, enum-match return-path analysis and CLI argument diagnostics.

## NEXUS Poker and graphics runtime polish (2026-10-26)

- Removed hover-driven cursor scaling and pulsing from the poker GUI so the custom pointer remains visually stable near button hitbox edges.
- Added a persistent cosmetic shop with Midnight Blue, Royal Purple and Crimson table themes. Skins are purchased with chips earned during play and do not affect poker rules or AI strength.
- Made `gfx_cursor_visible` idempotent in the runtime by caching the requested SDL visibility state and resetting the cache when graphics initializes.
- Updated poker example documentation and the 2.1.0 feature summary.

## NEXUS 2.1.0 release alignment and Poker scoring (2026-10-26)

- Aligned the graphical poker demo, its window/menu branding, and example documentation on NEXUS Poker 2.1.0.
- Corrected five-card hand scoring to compare all relevant kickers in order and to detect straight flushes only when all five cards share a suit; Ace-low straights are handled explicitly.
- Updated the Paint installer display name to NEXUS Paint 2.1.0 so its uninstall metadata matches its product version.

## Download reliability and site usability (2026-10-26)

- The Paint `.exe` download is a setup installer that installs the app and its SDL/SQLite runtime DLLs; the ZIP remains a portable app bundle.
- Switched the NEXUS installer away from the ultra/64 MB compression preset to a faster LZMA2 preset to reduce delays during publishing.
- Added pre-publish validation for every Windows/VS Code download: checksum matching, PE signature checks, ZIP/VSIX CRC checks, and required-content validation. The post-deploy workflow verifies every published download URL.
- Refreshed the landing page, downloads cards, responsive styles, navigation and light/dark colour modes.
- Fixed post-deploy PDF verification to check `/pdf/NEXUS-Documentation.pdf` instead of looking for the manual in `/assets/downloads/pdf/`.
- Published conventional SHA-256 files that include the matching asset filename, and made pre-publish validation compare the downloads-page links with the generated artifact inventory.
- Increased documentation text size and spacing, improved keyboard focus visibility, and made checksum links easier to identify.
- Added Unicode code-point iteration for `for codepoint in string` in the C++ semantic checker and LLVM backend, with executable UTF-8 round-trip, supplementary-plane, count and empty-string regression coverage.


## Unicode string escapes and language ergonomics (2026-10-26)

- Added `\u{HEX}` Unicode scalar escapes to the C++ and NEXUS self-hosted lexers; scalar values are validated and decoded to UTF-8.
- Added coverage for supplementary-plane characters, source-span accuracy, and invalid/forbidden Unicode scalar values.

## 2D game runtime and language ergonomics (2026-10-26)

- Added `stdlib/engine2d.nx`, a consistent convenience API over the existing graphics, texture, animation, input, camera, audio and AABB runtime functions.
- Added `examples/game2d_platformer.nx`, a no-assets platformer starter with frame-time clamping, keyboard movement, gravity, jumping and simple platform landing.
- Added `stdlib/math.nx` with clamp, interpolation, approach and sign helpers, plus an executable CTest smoke test.
- Added a compile-time API contract test for the 2D helper module and documented measurable engine/language roadmap milestones.
- Split the 2D game helpers into separate `std.engine2d`, `std.math`, `std.tilemap`, `std.tilemap_render`, `std.collision2d`, `std.animation2d`, `std.sprite2d`, `std.camera2d` and `std.audio2d` standard-library modules.
- `std.tilemap` owns grid storage; `std.collision2d` handles AABB/circle tests and exact axis-separated grid sweeps; `std.tilemap_render` owns viewport-aware colored drawing; `std.animation2d` provides time-based frame selection.
- Refactored the platformer to use these separate APIs and added an all-module executable regression smoke test plus edge-of-tile collision coverage.
- The modules are a foundation rather than a full engine: Tiled import/editor, per-tile collision metadata, texture atlases, a rigid-body solver, a general asset pipeline, integrated debugger and complete self-hosting remain future work.

## Runtime parsing and process safety (2026-10-26)

- Replaced text-search JSON extraction with a bounded parser that validates JSON grammar, decodes escape sequences and Unicode, checks raw UTF-8, and enforces input-size, nesting and node limits.
- Added `json_valid`, `json_type`, `json_array_len` and `json_array_get`, plus wrappers in `std.json` and regression coverage for malformed input, nested-key confusion, escaped strings, Unicode and array access.
- Changed the HTTP GET runtime to pass URLs as direct process arguments rather than through a shell, check the curl exit status, and cap captured bodies at 16 MiB.
- Added a regression test against shell interpretation of URL characters and updated runtime/tooling documentation and editor completions.

## Product site and developer workflow (2026-10-11)

- Redesigned the documentation landing page around a cleaner blue/yellow product identity and clearer download/start paths.
- Added explain-backend diagnostics for native eligibility and LLVM fallback decisions.
- Wired the displayed CLI version to the CMake project version.
- Added smoke tests for backend explanations and the version command, plus native/LLVM output parity tests on x86-64 Linux and Windows.
- Added a tag-triggered multi-platform release workflow for Linux x86-64, macOS Apple Silicon/Intel, and Windows x64, bundling Zig-based C++ toolchains and SHA-256 checksums.


## Unreleased - builtin native backend

- Added a builtin direct machine-code backend (`--backend native`, default via `--backend auto`): x86-64 Windows PE and Linux ELF executables with no LLVM, Clang, assembler, or linker involved.
- Supported subset: `i64`/`bool`/`string` programs with functions, control flow, `print`, `input`, `str_i64`, and string concatenation/comparison; strings live in a 16 MiB startup bump arena.
- Non-subset programs are rejected with precise reasons and fall back to the LLVM backend under `auto`.
- Added `nexus export` (release binary + staged directory + optional zip archive).
- Child processes now launch via `CreateProcessW`/`fork+exec` instead of shell strings, so compiler and project paths with spaces work.
- `nexus doctor` no longer fails without `clang++`; it reports each backend separately.
- Added regression (`tests/regression_programs`), native end-to-end (`tests/native_programs`), portable packaging (`.github/project-tools/scripts/package_portable.cmake`), Linux/macOS `.github/project-tools/installer/install.sh, and `THIRD_PARTY_NOTICES.md`.
- CI builds and runs the native battery with compilers removed from `PATH` (Windows/Linux) and checks graceful fallback on macOS.

## Packaging update (2026-10-10)

- Replaced fragile `cmd.exe` variable expansion with a PowerShell-based Windows launcher.
- Added native compiler-toolchain detection and an actual compile/run validation step.
- Added a Windows portable ZIP with drag-and-drop `.nx` launch and `.exe` build scripts.
- Added the Windows ZIP and SHA-256 checksum to the MkDocs site downloads section.
- Fixed GitHub Pages dependency installation to include `mkdocs-minify-plugin`.
- Excluded `.git`, build caches, and generated website files from the source-release bundle.

## 1.1.0 - Graphics backend rewrite

- SDL2 2D renderer is now the default for windowed applications and Nexus Paint.
- 2D rendering no longer depends on an OpenGL compatibility profile.
- Canvas creation/destruction is backend-safe.
- Resize, maximize and fullscreen APIs are exposed to Nexus.
- Virtual UI coordinates remain stable across window sizes.
- `gfx_backend()` reports the active renderer.
- Runtime graphics code is organized into explicit 2D, 3D, canvas, input and window sections.
- OpenGL is activated only when a 3D frame is requested.
- `ctest` and native graphics smoke tests are part of the release validation.
- `usize`/`isize` aliases map to the platform integer width supported by the current backend.
- Added low-level `mem_set` and `mem_copy` runtime operations alongside `mem_alloc`/`mem_free`.

