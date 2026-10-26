# Changelog

## Download reliability and site usability (2026-10-26)

- The Paint `.exe` download is a setup installer that installs the app and its SDL/SQLite runtime DLLs; the ZIP remains a portable app bundle.
- Switched the NEXUS installer away from the ultra/64 MB compression preset to a faster LZMA2 preset to reduce delays during publishing.
- Added pre-publish validation for every Windows/VS Code download: checksum matching, PE signature checks, ZIP/VSIX CRC checks, and required-content validation. The post-deploy workflow verifies every published download URL.
- Refreshed the landing page, downloads cards, responsive styles, navigation and light/dark colour modes.


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

# Changelog

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

