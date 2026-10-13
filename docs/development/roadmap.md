# NEXUS Roadmap

This roadmap distinguishes shipped behavior from goals. A directory, example, or API facade is not evidence that a feature is complete.

## Available today

- C++20 compiler with lexer, parser, semantic analysis, LLVM IR generation, and a Clang-based native fallback.
- Built-in x86-64 machine-code emitter for a documented subset of programs on supported Windows/Linux hosts. It does not replace the LLVM fallback for the full language.
- Project templates and CLI workflows for creating, checking, building, running, testing, formatting, linting, exporting, and packaging projects.
- Runtime modules for file and input operations, networking helpers, JSON field extraction, SQLite access, SHA-256, memory helpers, SDL-based graphics, and audio. APIs remain limited to what is implemented in the runtime.
- LSP transport with diagnostics/completion, built-in and user-declaration hover, lexical references/rename and whole-document formatting, plus a limited go-to-definition resolver for currently open documents; there is no global workspace index yet.
- This release's environment doctor, JSON diagnostic output, built-in compile/run self-test, shared CLI/LSP whole-document formatter, and deterministic frontend fuzz smoke test.
- The GitHub Actions matrix builds/tests Ubuntu, Windows, and macOS hosts. A separate Linux workflow runs ASan/UBSan on compiler and runtime tests; refer to the [platform support matrix](platform-support.md) for output-target boundaries.

## Priority 1 — Trustworthy development workflow

1. Keep the full test suite green on clean Linux, Windows, and macOS runners; run the sanitizer workflow for frontend/runtime changes.
2. Make `nexus doctor` verify the actual packaged compiler/runtime/stdlib/template layout on each platform.
3. Extend tests for paths with spaces, clean installs, missing optional tools, malformed source, and repeatable builds.
4. Make formatter and linter behavior deterministic, then support project-wide file discovery with explicit exclusions.
5. Keep the CLI and published documentation synchronized so examples only use implemented options.

**Completion criterion:** a fresh checkout or portable package passes build, test, and the built-in native compile/run self-test without relying on files from a developer's machine.

## Priority 2 — Language-server and package reliability

1. Extend the current declaration resolver and open-document reference/rename support into a real symbol index covering open and unopened files, with scope-aware navigation.
2. Add type-aware hover, scope-aware references/rename, range formatting and workspace-wide symbol indexing to the LSP, with regression tests. Built-in/keyword and parsed user-declaration hover, lexical references/rename in open documents and whole-document formatting are covered by integration tests.
3. Strengthen lockfile behavior with atomic writes, preserved unrelated entries and tests for malformed manifests.
4. Add integrity checksums and reproducible vendoring while keeping package path validation and transient-file exclusions.
5. Expand diagnostics with stable error codes, precise source spans and multi-error recovery.

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
