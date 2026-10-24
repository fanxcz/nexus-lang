# Contributing to NEXUS

Thank you for helping improve NEXUS. The compiler, language, standard library, runtime, examples and documentation are maintained in this repository.

## Before changing code

Check the [roadmap](https://github.com/fanxcz/nexus-lang/blob/main/docs/development/roadmap.md), open issues and recent commits before starting a large change. For substantial language or runtime changes, open an issue first so the design can be discussed.

Keep changes focused. Add a regression test for every compiler bug or language feature where practical. Document language limitations and target support honestly: a listed or detectable target is not automatically supported by every backend.

## Build and test

Requirements: CMake 3.25+ for the checked-in presets, Ninja, and a C++20 compiler. The LLVM path additionally needs a supported Clang/LLVM toolchain. Graphics/runtime tests may need platform SDL2 libraries.

```bash
git clone https://github.com/fanxcz/nexus-lang.git
cd nexus-lang
cmake --preset=debug
cmake --build --preset=debug
ctest --test-dir build --output-on-failure
./build/nexus --version
./build/nexus self-test
```

On Windows, use `build/nexus.exe` instead of `build/nexus`. With CMake 3.20–3.24, use explicit configure commands instead of presets:

```bash
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

For a change to language behavior, add a focused `.nx` test under the appropriate `tests/` directory and run the CTest suite. For docs-only changes, verify all relative links and examples against the current implementation.

## Code and documentation

- Keep the compiler's frontend, semantic analysis, backend and runtime responsibilities separated.
- Use the repository's existing naming, ownership and error-handling conventions for C++.
- Keep NEXUS code examples runnable where possible; identify any example that is illustrative rather than executable.
- Do not silently change language semantics or public runtime signatures.
- Update the relevant reference pages, examples and changelog for user-visible changes.
- Never commit build output, generated binaries, private data, API tokens or unrelated files.

## Suggested workflow

```bash
git checkout -b fix/short-description
# make a focused change
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
git status --short
git add <files>
git commit -m "fix: describe the change"
git push -u origin fix/short-description
```

Open a pull request against `main` and include the problem, the approach, tests run, and any known limitations. Do not report a test as passing unless it was run and passed.

## Pull request checklist

- [ ] The change is focused and explained.
- [ ] Tests cover the behavior changed.
- [ ] Relevant tests and `git diff --check` pass.
- [ ] Documentation and examples match the implementation.
- [ ] No generated build output or secrets are included.
- [ ] Backend and platform claims are supported by real builds/tests.

## Where to help

Start with [open issues](https://github.com/fanxcz/nexus-lang/issues), or the [development documentation](https://github.com/fanxcz/nexus-lang/blob/main/docs/development/index.md). Bug reports should include a minimal source example, exact command, platform, expected behavior and actual output.
