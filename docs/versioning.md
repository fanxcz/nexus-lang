# Versioning Policy

NEXUS uses Semantic Versioning (`MAJOR.MINOR.PATCH`) for compiler releases and the language contract. Before `1.0.0`, compatibility guarantees may be adjusted explicitly in release notes; the current project is versioned `2.x`.

- **Major** (`X.0.0`): breaking language syntax or semantics, removal/incompatible change to public standard-library or runtime APIs, or compiler behavior that requires source migration.
- **Minor** (`2.Y.0`): backward-compatible language additions, new builtins, standard-library functionality, backend capabilities or CLI commands that do not break existing programs.
- **Patch** (`2.Y.Z`): backward-compatible bug fixes, diagnostics, build/release fixes and documentation corrections.

A feature promoted from experimental to stable must state its compatibility promise. Backend support is part of the release contract: a language feature may be supported by LLVM before the builtin native backend. Such differences must be documented in the changelog and [backend support matrix](backend-support-matrix.md).

## Release process

1. Update `project(VERSION ...)` in `CMakeLists.txt` and any intentionally derived version sources.
2. Move completed entries from `Unreleased` in `CHANGELOG.md` into a dated release section.
3. Run CMake build, CTest, formatting, negative, differential and available fuzz checks.
4. Create an annotated `vX.Y.Z` tag from the reviewed `main` commit.
5. Publish platform artifacts with SHA-256 checksum files; keep the source tag and release notes aligned.

Never change source compatibility silently in a patch release. Deprecations should be documented for at least one minor release before removal where practical.
