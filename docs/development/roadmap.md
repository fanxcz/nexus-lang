# NEXUS Roadmap

This roadmap separates implemented behavior from planned work. A wrapper, directory,
demo or API name is not proof that a complete subsystem exists; completion means the
feature is implemented, documented and covered by tests on the platforms it claims
to support.

## Product direction

NEXUS is focused first on **native desktop applications, command-line utilities and
developer tools**. Graphics and games remain supported use cases. The near-term goal
is a dependable developer experience: understandable compiler errors, predictable
builds, useful standard-library functions and reliable packaging.

## What exists today

- **Compiler:** C++20 lexer, parser, AST, semantic analysis, LLVM IR generation and a
  built-in x86-64 code emitter for a documented subset. The LLVM path uses an available
  Clang-compatible toolchain for native linking.
- **Language:** fixed-size arrays, structs, enums with one supported scalar payload,
  pointers, imports, loops, pattern matching, `defer` on the LLVM path, and concrete
  enum error propagation with `?` on the LLVM path. Generic types, generic
  `Option<T>`/`Result<T, E>`, ownership checking and cross-enum error conversion are
  not implemented.
- **CLI and project workflow:** project templates, check/build/run, formatting,
  linting, tests, project inspection, asset listing, packaging workflows, package
  manifest/lockfile support for local dependencies and an environment doctor.
- **Editor tooling:** a Language Server Protocol implementation with diagnostics,
  completion, hover, signature help, document symbols, workspace symbol search,
  navigation, references, rename, formatting and quick-fix actions. Cross-file type
  inference and import-alias resolution remain limited.
- **Standard library:** file helpers including checked reads, strings, typed integer
  and string lists, math, JSON inspection, HTTP GET, SQLite operations, SHA-256,
  memory helpers, concrete Option/Result helper enums, and UI/application helpers.
  The APIs are narrower than mature standard libraries in other languages.
- **Runtime:** SDL-backed drawing, window/input handling, textures, fonts, optional
  audio, immediate-mode UI widgets, basic ECS/scene helpers, particles and simple
  collision functions. Optional host libraries affect runtime availability.
- **2D modules:** `std.vec2`, `std.vec3`, `std.engine2d`, `std.tilemap`,
  `std.tilemap_render`, `std.collision2d`, `std.physics2d`, `std.animation2d`,
  `std.sprite2d`, `std.camera2d` and `std.audio2d` provide small composable
  helpers. They are not a complete game engine or rigid-body solver.
- **Verification:** CI build/test jobs cover Ubuntu, Windows and macOS; a separate
  Linux workflow runs AddressSanitizer and UndefinedBehaviorSanitizer tests. Check
  the linked workflow runs for the status of the latest commit rather than assuming
  every job has already completed.

## Milestone 1 — Reliability before feature breadth

**Goal:** a clean checkout and a freshly installed compiler behave predictably.

1. Keep compiler tests, runtime tests, regression programs and formatting checks green
   across the supported CI matrix.
2. Exercise spaces in project paths, missing optional dependencies, malformed files,
   failed resource loads and unavailable audio/graphics backends.
3. Ensure `nexus doctor`, installation packages and portable bundles describe and
   validate the actual compiler, standard library, templates and runtime dependencies.
4. Make CLI parsing strict and consistent: unsupported options and malformed numeric
   arguments must fail with a clear message rather than being silently ignored.
5. Validate newly added APIs with executable smoke/regression tests before advertising
   them in the manual or README.

**Done when:** a clean checkout builds and passes tests on the declared host matrix,
the packaged compiler passes its self-test, and the download/package workflow verifies
that the files it publishes are present and usable.

## Milestone 2 — A practical native-app workflow

**Goal:** NEXUS can comfortably build small desktop tools rather than only drawing demos.

1. Extend the basic `std.ui` text field with stable widget IDs, caret/selection editing,
   Escape-to-cancel, full multi-field focus traversal, consistent control states and UI scaling.
2. Add OS integration through tested APIs: file dialogs, clipboard, application
   configuration directories, process launching and clear error results.
3. Establish consistent conventions for return codes, handles, missing values and
   error messages across filesystem, JSON, network and database helpers.
4. Provide a multi-file example application that exercises UI controls, persistence,
   file operations and clean shutdown.
5. Make resource copying and packaging reproducible, with a test that launches the
   packaged app from a clean directory without depending on the source checkout.

**Done when:** a documented multi-file desktop utility can be built and packaged on
a declared supported host, and the packaged program runs from a clean location.

## Milestone 3 — Language ergonomics and compiler correctness

**Goal:** improve the language through small, coherent features that have precise semantics.

1. Keep diagnostics source-aware and actionable. Add regression tests for each semantic
   bug, especially control-flow analysis, match guards, mutability, pointer operations,
   type mismatches and invalid literals.
2. Define the behavior of integer overflow, floating-point edge cases, array bounds,
   pointer use and runtime failure before expanding syntax.
3. Document the behavior shared by the LLVM and built-in native backends. If one
   backend cannot implement a feature, it must report the limitation explicitly.
4. Design a generic type system and ownership/lifetime model as language proposals,
   not as library wrappers that imply unsupported semantics.
5. Consider a richer, generic `Result<T, E>`/`Option<T>` design only after enum
   payload layout, propagation rules, diagnostics and backend support have a coherent
   cross-backend specification.

**Done when:** language changes have parser/semantic/codegen tests, negative tests,
clear documented behavior, and backend-specific support notes.

## Milestone 4 — Standard library and app services

**Goal:** make the library practical, consistent and safe to consume.

1. Document every public function's inputs, outputs, ownership expectations, failure
   sentinel and platform limits.
2. Expand filesystem, process, environment, HTTP, JSON and database support through
   real runtime implementations rather than thin placeholder wrappers.
3. Give HTTP requests explicit success/error status and stop conflating an empty body
   with a failed request.
4. Expand JSON operations incrementally toward structured values, typed conversions
   and serialization, each with malformed-input and size-limit tests.
5. Prefer checked collection operations and explicit resource-release functions.
   Generic containers and automatic ownership remain future language/runtime work.

**Done when:** each advertised API has runnable examples and tests for invalid input,
failure paths and the platforms on which it is claimed to work.

## Secondary track — 2D gameplay and graphics helpers

**Goal:** keep a useful, self-contained 2D toolkit available without misrepresenting
it as a commercial engine.

1. Keep the asset-free platformer starter compiling from a clean checkout and verify
   movement, jumping, collisions, camera transforms and orderly shutdown.
2. Expand tilemap import/authoring, per-tile flags, texture-atlas lifecycle and
   deterministic resource cleanup where tests can cover the behavior.
3. Strengthen the animation, sprite, camera, audio and collision regression suite,
   especially frame stalls, large deltas, invalid handles and missing files.
4. Treat rigid-body physics, advanced 3D materials, shaders and a full scene editor
   as separate projects with their own architecture and test plans.

**Done when:** the documented 2D example builds and runs without external art assets,
uses implemented APIs only, handles the main interaction loop and cleans up resources.

## Milestone 5 — Editor and package ecosystem

**Goal:** reduce the friction of building and maintaining real projects.

1. Improve LSP correctness and performance on larger workspaces, especially imported
   aliases, cross-file type resolution and cache invalidation.
2. Keep completions and quick-fixes consistent with compiler behavior; do not suggest
   syntax or APIs that the selected backend cannot compile.
3. Continue testing local package manifests, lockfile updates, path confinement and
   checksums. Consider an online registry only after local dependency behavior is stable.
4. Improve the VS Code extension and project templates against clean-machine install
   tests.

**Done when:** editor navigation and package operations are deterministic and tested
against the same compiler/library layout that ships with NEXUS.

## Long-term projects

These are not implied to be implemented by the current source tree:

- Generic type parameters, traits/interfaces, function values and closure lowering.
- A checked ownership/lifetime model or a fully specified alternative.
- A self-hosted production compiler and broad native-backend feature parity.
- More cross-compilation targets, including WebAssembly and mobile targets.
- A retained, accessible, cross-platform desktop UI framework.
- A full game engine with rigid-body physics, editor tooling, asset pipelines and
  advanced rendering.

Each project needs a design proposal, explicit support boundaries, implementation,
tests and release validation. The priority is to keep NEXUS useful and predictable
while its capabilities grow.
