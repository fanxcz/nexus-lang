# Testing NEXUS

NEXUS uses C++ unit tests, CMake-script integration tests, and end-to-end compiler/runtime smoke tests. Run the suite after changing the compiler, runtime, package layout, editor integration, or documentation tooling.

## Build and run the suite

```sh
cmake --preset=debug
cmake --build --preset=debug
ctest --test-dir build --output-on-failure
```

To run one CTest by name:

```sh
ctest --test-dir build -R nexus_lsp_smoke --output-on-failure
ctest --test-dir build -R nexus_native_smoke --output-on-failure
```

## What the suite covers

| Test | Purpose |
|---|---|
| `nexus_tests` | C++ lexer/parser/semantic/compiler unit checks |
| `nexus_runtime_memory_test` | Runtime memory behavior |
| `nexus_runtime_json_test` | JSON grammar, escape decoding, Unicode, bounded parsing, top-level lookup and array access |
| `nexus_runtime_http_test` | Ensures URL input is passed to the HTTP client without shell interpretation |
| `nexus_version_smoke` | CLI version output |
| `nexus_self_test` | Compile a tiny program and verify its expected exit code |
| `nexus_doctor_json_smoke` | Parse the doctor response and validate required JSON field types |
| `nexus_vscode_extension_smoke` | Validate VS Code extension metadata, grammar, and snippets |
| `nexus_backend_explain_smoke` | Backend-selection explanation |
| `nexus_project_smoke` | Project creation/build workflow, formatter check mode, and VS Code settings |
| `nexus_project_paths_smoke` | Create, format, lint, build and run a project whose directory and artifact names contain spaces |
| `nexus_corrupt_inputs_smoke` | Malformed `nexus.toml` manifests, control bytes in sources, missing entry files and directory arguments fail with actionable diagnostics |
| `nexus_templates_smoke` | Every shipped template either scaffolds a project that passes `nexus check` or is rejected as an unknown template |
| `nexus_runtime_backend_state_test` | Graphics/audio entry points return documented sentinels instead of crashing when no backend was initialized |
| `nexus_fetch_installer_smoke` | The installer downloader rejects HTML error pages and checksum mismatches and accepts a verified PE image |
| `nexus_paint_smoke` | Paint template compile/run smoke checks |
| `nexus_defer_try_propagation_smoke` | Verifies `?` error returns execute all deferred cleanups in reverse order |
| `nexus_diagnostics_smoke` | Error/diagnostic formatting |
| `nexus_lsp_smoke` | Actual LSP initialize, document-change, completion, definition, formatting and shutdown messages |
| `nexus_frontend_fuzz_smoke` | Runs 5,000 deterministic generated inputs through the lexer and parser to catch crashes and hangs; malformed input is an expected outcome |
| `nexus_regression_check` | Selected regression programs |
| `nexus_native_smoke` | Built-in machine-code backend execution, on supported platforms |
| `nexus_backend_parity` | Compare built-in and LLVM backend behavior where available |

The exact platform-specific test list depends on the CMake options and host platform. CI builds and tests on Ubuntu, Windows, and macOS. The built-in direct machine-code backend is focused on the supported x86-64 Windows/Linux subset; platform jobs may exercise different smoke tests.

CI runners do not install SDL2, so every leg also builds and tests the compiler with the graphics and audio backend unavailable. `nexus_runtime_backend_state_test` pins that degraded path: drawing calls are no-ops, `nexus_gfx_backend()` reports `uninitialized`, and a sound file that cannot be loaded returns the `0` sentinel with a recorded error message.

## Sanitizer checks

The `.github/workflows/sanitizers.yml` workflow builds the compiler, frontend fuzz smoke test, and runtime tests with Clang AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan). Use it for changes to the lexer, parser, compiler driver, or runtime. The normal three-host CI remains the primary portability check.

To reproduce a sanitized build locally on Linux with Clang installed:

```sh
cmake -S . -B build-sanitize -G Ninja \
  -DNEXUS_BUILD_TESTS=ON \
  -DNEXUS_USE_CLANG=OFF \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-sanitize --parallel 2
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-sanitize --output-on-failure
```

The generated frontend cases use a fixed seed so a failure can be reproduced. This is a deterministic stress smoke test, not a substitute for coverage-guided fuzzing.

## Project-level NEXUS tests

The `nexus test` command is separate from CTest. Run it from a NEXUS project with a `nexus.toml` manifest. It discovers `.nx` files recursively under `tests/programs/`, compiles each with the built-in native backend, and runs it. Every test program must exit with status 0 to pass. There is currently no test-attribute discovery or `--filter` option; a missing `tests/programs/` directory is reported as no project tests.

The compiler repository's CMake integration tests live directly under `tests/` as `.cmake` scripts, with C++ test code alongside them. When adding a feature, add a regression case at the level that exercises the actual contract: parser/semantic unit test, end-to-end compile/run test, LSP protocol test, or packaging/configuration smoke test.

## Useful checks

```sh
# Verify the command line environment
./build/nexus doctor --json

# Verify compile-and-run behavior
./build/nexus self-test

# Run all CMake tests
ctest --test-dir build --output-on-failure
```

Avoid describing a feature as implemented until it is covered by a test and passes on the platforms listed as supported. Consult the [development roadmap](roadmap.md) for current priorities and limitations.
## Repository-wide source integrity audit

Run the repository preflight before changing or packaging the project:

```sh
python scripts/audit_repository.py
```

The audit inventories every Git-tracked path, verifies that files are present, checks recognized text files for UTF-8 validity and unresolved merge-conflict markers, parses all tracked JSON and Python files, syntax-checks tracked JavaScript with Node.js, and checks stylesheet block structure. CI runs this before the compiler build on Ubuntu, Windows, and macOS.

This preflight is intentionally lightweight. It complements rather than replaces the CMake/CTest suite, sanitizer runs, VS Code and Studio checks, Markdown link checks, strict MkDocs build, and download/package validation.
