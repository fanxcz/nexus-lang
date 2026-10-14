# NEXUS Roadmap

This roadmap distinguishes shipped behavior from goals. A directory, example, or API facade is not evidence that a feature is complete.

## Available today

- C++20 compiler with lexer, parser, semantic analysis, LLVM IR generation, and a Clang-based native fallback.
- Built-in x86-64 machine-code emitter for a documented subset of programs on supported Windows/Linux hosts. It does not replace the LLVM fallback for the full language.
- Project templates and CLI workflows for creating, checking, building, running, testing, formatting, linting, exporting, and packaging projects.
- Runtime modules for file and input operations, networking helpers, JSON field extraction, SQLite access, SHA-256, memory helpers, SDL-based graphics, and audio. APIs remain limited to what is implemented in the runtime.
- LSP transport with completion, hover, diagnostics, whole-document and selected-range formatting, and quick-fix code actions; per-workspace source caching and per-file lexical indexes support go-to-definition, references and rename across open and unopened `.nx` files. Local variables, loop variables and simple parameters use brace-scope-aware resolution; global functions, structs and enums are indexed across files. Prefix-filtered completion and context-aware struct-field/enum-variant suggestions are implemented. Parsed AST nodes and semantic diagnostics retain source ranges; import-alias resolution and fully inferred cross-file field typing remain future work.
- This release's environment doctor, JSON diagnostic output, built-in compile/run self-test, token-accurate AST source spans (including escaped string literals), shared CLI/LSP whole-document formatter with comment/string-aware token spacing and regression coverage for decimal literals, strings, comments, parseability and idempotency, top-level parser recovery that reports multiple syntax errors, and deterministic frontend fuzz smoke test.
- The GitHub Actions matrix builds/tests Ubuntu, Windows, and macOS hosts. A separate Linux workflow runs ASan/UBSan on compiler and runtime tests; refer to the [platform support matrix](platform-support.md) for output-target boundaries.

## Priority 1 — Trustworthy development workflow

1. Keep the full test suite green on clean Linux, Windows, and macOS runners; run the sanitizer workflow for frontend/runtime changes.
2. Make `nexus doctor` verify the actual packaged compiler/runtime/stdlib/template layout on each platform.
3. Extend tests for paths with spaces, clean installs, missing optional tools, malformed source, and repeatable builds.
4. Keep formatter and linter behavior deterministic. `nexus fmt --all` and `nexus fmt --check --all` now process `.nx` files in sorted path order while skipping dependency and build-output directories.
5. Keep the CLI and published documentation synchronized so examples only use implemented options.

**Completion criterion:** a fresh checkout or portable package passes build, test, and the built-in native compile/run self-test without relying on files from a developer's machine.

## Priority 2 — Language-server and package reliability

1. Benchmark cache refresh behavior on larger repositories and add regression coverage for file-watcher invalidation after external edits, renames, and deletions.
2. Extend semantic navigation to imported aliases and more inferred field types; build on the current selected-range formatter and field quick fixes with broader editor actions and regression tests.
3. Keep atomic lockfile writes, preserve unrelated manifest entries, and add malformed-manifest regressions.
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
