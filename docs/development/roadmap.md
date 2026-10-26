# NEXUS Roadmap

This roadmap distinguishes shipped behavior from goals. A directory, example, or API facade is not evidence that a feature is complete.

## Product direction: native applications first

The main product goal is to make NEXUS a dependable language for building native desktop applications, command-line utilities, and developer tools. Games and graphics remain useful examples and optional use cases, but should not drive every language or runtime decision.

## Priority — Native application workflow

1. Make the app template a reliable starting point for real tools, with a clear project structure and documented event/window lifecycle.
2. Build a consistent desktop UI layer incrementally: layout, buttons, text inputs, checkboxes, menus, keyboard focus, scaling, and accessible keyboard navigation. Do not present drawing primitives alone as a mature widget toolkit.
3. Expand platform services through tested runtime implementations: file dialogs, clipboard, configuration paths, environment access, process launching, and predictable error reporting.
4. Make resource and dependency packaging reproducible so an app can be built into a distributable folder or installer without relying on the repository checkout or manually copied files.
5. Add end-to-end example apps that test a practical workflow (for example, a small notes or file-inspection tool) from a clean install through build, launch, and shutdown.

**Completion criterion:** a multi-file desktop utility can be created from the documented template, built on a declared supported host, and packaged with its runtime resources so it works on a clean machine. Each advertised platform needs its own CI or release-artifact validation.
## Available today

- C++20 compiler with lexer, parser, semantic analysis, LLVM IR generation, and a Clang-based native fallback. Acyclic source imports load once per compilation; circular import chains are rejected with a path trace.
- Built-in x86-64 machine-code emitter for a documented subset of programs on supported Windows/Linux hosts. It does not replace the LLVM fallback for the full language.
- Project templates and CLI workflows for creating, checking, building, running, testing, formatting, linting, exporting, and packaging projects.
- Runtime modules for file and input operations, networking helpers, a bounded JSON parser (validation, top-level property extraction/type inspection and top-level array access), SQLite access, SHA-256, memory helpers, SDL-based graphics, and audio. HTTP still depends on the system `curl` executable and does not expose status codes/headers; JSON schema validation, typed conversions and serialization remain future work. APIs remain limited to what is implemented in the runtime.
- `file_read_checked(path, &out)` and `std.fs.fs_read_checked` distinguish a valid empty file from a read failure. This is an explicit status/out-parameter API. Concrete payload enums and `?` propagation are implemented on the LLVM path; generic `Result<T, E>` / `Option<T>` types and cross-enum error conversion remain unimplemented.
- `std.collections` exposes checked reserve/push/insert/get/set/remove/pop/clear/free operations for integer and string lists. The handles are now explicitly releasable; these specialized wrappers do not yet constitute generic `Vec<T>` or a compiler-supported ownership model.
- LSP transport with completion, hover, signature help, diagnostics, whole-document and selected-range formatting, quick-fix code actions, current-file Outline (`textDocument/documentSymbol`), and workspace-wide declaration search (`workspace/symbol`, capped at 500 results). Signature help displays parameter names/types for user functions across the workspace and known built-ins, tracking the active parameter through nested calls. Per-workspace source caching and per-file lexical indexes support go-to-definition, references and rename across open and unopened `.nx` files. Local variables, loop variables and simple parameters use brace-scope-aware resolution; global functions, structs and enums are indexed across files. Prefix-filtered completion and context-aware struct-field/enum-variant suggestions are implemented, including inferred chained access such as `person.home.` and workspace-return-type chains such as `make_person().address.` when each link has an indexed struct type. Parsed AST nodes and semantic diagnostics retain source ranges; import-alias resolution and fully inferred cross-file field typing remain future work.
- This release's environment doctor, JSON diagnostic output, built-in compile/run self-test, token-accurate AST source spans (including escaped string literals), shared CLI/LSP whole-document formatter with comment/string-aware token spacing, token-stream preservation checks for already-parseable input, and regression coverage for decimal literals, escaped strings, comments, adjacent unary operators, parseability and idempotency, top-level parser recovery that reports multiple syntax errors, and deterministic frontend fuzz smoke test.
- Lexical `defer` is implemented on the LLVM backend for void-returning calls, with argument capture at registration and LIFO cleanup on normal exit, `return`, `break`, and `continue`; the builtin native subset deliberately reports it as unsupported until parity is implemented.
- The GitHub Actions matrix builds/tests Ubuntu, Windows, and macOS hosts. A separate Linux workflow runs ASan/UBSan on compiler and runtime tests; refer to the [platform support matrix](platform-support.md) for output-target boundaries.

## Secondary domain — 2D game runtime and coherent APIs

1. Build on the existing SDL-backed renderer, texture loading, animation, input, camera, audio and AABB primitives instead of presenting them as a complete engine.
2. Keep a small consistent 2D convenience module in `stdlib/engine2d.nx`; add regression examples that can be built from a clean checkout and use only shipped APIs.
3. Gameplay helpers now have separate `std.tilemap`, `std.tilemap_render`, `std.collision2d`, `std.animation2d`, `std.sprite2d`, `std.camera2d` and `std.audio2d` modules. Continue with Tiled import, per-tile flags, texture-atlas lifecycle and stronger physics tests. The runtime `physics_aabb` helper remains overlap-only and is not a rigid-body solver.
4. Test frame timing under stalls, keyboard input, missing assets and unavailable audio/graphics libraries on supported platforms.

**Completion criterion:** a documented platformer example builds with the shipped compiler, runs without external assets, handles movement/jumping and collisions, and shuts down cleanly.

## Priority 4 — Language and standard-library ergonomics

1. Add small, well-tested standard-library helpers before expanding syntax or the type system; keep the APIs useful to general applications as well as graphics/simulation code.
2. Add tests for numeric edge cases (negative values, exact boundaries, reversed ranges and non-finite inputs where supported) and ensure examples use declared mutable bindings for state.
3. Prioritize clear diagnostics, deterministic formatting and self-host compiler parity before introducing broad syntax changes.
4. Track classes/inheritance, richer enums, dynamic arrays and ownership/lifetime design as separate language proposals; do not imply these features are implemented by adding facade APIs.

**Completion criterion:** standard-library additions have executable regression coverage, are documented, and pass the same clean-checkout CI matrix as the compiler.

## Priority 1 — Trustworthy development workflow

1. Keep the full test suite green on clean Linux, Windows, and macOS runners; run the sanitizer workflow for frontend/runtime changes.
2. Make `nexus doctor` verify the actual packaged compiler/runtime/stdlib/template layout on each platform.
3. Extend tests for paths with spaces, clean installs, missing optional tools, malformed source, repeatable builds, and interactive-terminal editing, history navigation, and Tab completion.
4. Keep formatter and linter behavior deterministic. `nexus fmt --all` and `nexus fmt --check --all` now process `.nx` files in sorted path order while skipping dependency and build-output directories.
5. Keep the CLI and published documentation synchronized so examples only use implemented options.

**Completion criterion:** a fresh checkout or portable package passes build, test, and the built-in native compile/run self-test without relying on files from a developer's machine.

## Priority 2 — Language-server and package reliability

1. Benchmark cache refresh behavior on larger repositories; the LSP regression suite now exercises watcher invalidation after external edits, renames, and deletions.
2. Extend semantic navigation to imported aliases and more inferred field types; build on the current selected-range formatter and field quick fixes with broader editor actions and regression tests.
3. Keep atomic lockfile writes and preserve unrelated manifest entries. `pkg verify` now rejects malformed dependency entries, duplicate keys/tables, invalid lockfile headers, malformed lock records, and stale project metadata; keep these cases covered by smoke tests.
4. Verify local vendored packages against SHA-256 tree checksums; `pkg verify` cross-checks manifest/lockfile dependencies and versions, rejects stale records, and confines resolved paths to the project's `vendor/` directory. Registry trust and signed packages remain future work.
5. Expand diagnostics with stable error codes and semantic error recovery. AST expression/statement spans, aggregate syntax recovery, and multi-error semantic reporting for declarations and function bodies are implemented.

**Completion criterion:** editor navigation and package operations have deterministic behavior covered by tests.

## Priority 3 — Standard library depth

1. Document every public runtime function, ownership rule, error result, and platform limitation.
2. Expand filesystem, process, environment, HTTP, JSON, and database APIs through tested runtime implementations rather than thin wrappers alone.
3. Add platform parity tests and explicit capability checks when an API cannot work on a target.
4. Define stable error/result conventions before adding large numbers of new APIs.

**Completion criterion:** each advertised API has a real implementation, examples, and tests on the platforms it claims to support.

## Priority 4 — Native compiler independence

1. Expand the built-in x86-64 backend using the existing backend-parity and native smoke tests.
2. Add a documented intermediate representation boundary and benchmark changes against the current LLVM path.
3. Improve optimization only when benchmarks show a repeatable win and regression tests preserve behavior.
4. Investigate additional native targets only alongside executable-format, ABI, and CI coverage.
5. Start self-hosting as a separate milestone after the language and compiler interfaces stabilize.

**Important:** the built-in emitter is currently a subset backend. A complete dependency-free compiler for every language feature and operating system is not yet available.

## Priority 5 — Editor and ecosystem

1. Improve the current LSP before building a full editor.
2. Ship a small syntax-highlighting/editor extension and test installation instructions on clean machines.
3. Make project generation, formatting, linting, test execution, and release packaging discoverable from the editor.
4. Consider a package registry only after local packages and lockfiles are reliable.
5. Treat WebAssembly, Android, and additional desktop targets as separate projects with target-specific runtimes and CI.

## Non-goals for a quick patch

A self-hosting compiler, complete IDE, safe memory/ownership system, package registry, and universal zero-dependency cross-compiler cannot be completed responsibly by adding placeholders. Each needs implementation, tests, release artifacts, and an explicit supported-platform matrix.
