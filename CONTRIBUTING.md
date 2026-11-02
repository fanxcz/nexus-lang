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

Start with [open issues](https://github.com/fanxcz/nexus-lang/issues), or the [development documentation](docs/development/index.md). Bug reports should include a minimal source example, exact command, platform, expected behavior and actual output.

## Project engineering guides

Before changing language behavior, read the [compiler architecture](docs/architecture.md), [backend support matrix](docs/backend-support-matrix.md), and [NEXUS style guide](docs/style-guide-nx.md). Build and setup problems are collected in [Troubleshooting](docs/troubleshooting.md). Keep backend claims evidence-based; mark incomplete coverage as `Needs verification`.

The normal build remains free of libFuzzer requirements. Clang users can enable the optional lexer/parser fuzz targets explicitly:

```sh
cmake -S . -B build-fuzz -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNEXUS_BUILD_TESTS=ON -DNEXUS_BUILD_LIBFUZZER=ON
cmake --build build-fuzz --target nexus nexus_fuzz_lexer nexus_fuzz_parser
ctest --test-dir build-fuzz --output-on-failure
```

Run C++ format and static-analysis checks locally when tools are installed:

```sh
clang-format --dry-run --Werror <changed-cpp-files>
clang-tidy -p build <changed-cpp-files>
```

Do not silence a failing quality gate without documenting why. The [good first issues](docs/good-first-issues.md) page lists scoped starter tasks; use the `good first issue` and `help wanted` labels only after a maintainer confirms scope and acceptance criteria.

## Issues and Discussions

Use Issues for reproducible defects, test failures and concrete implementation tasks. Use Discussions for questions, ideas and early RFC exploration. Enable the feature using the [GitHub repository settings guide](docs/github-settings.md) if it is not available yet. The maintainer may request an RFC before substantial syntax, semantic, standard-library or backend-contract changes. Read [GOVERNANCE.md](GOVERNANCE.md) and [repository settings](docs/github-settings.md) for the proposal process and one-time Discussions setup.

## CI expectations

Pull requests are expected to pass the existing build/CTest workflow and applicable quality gates: C++ formatting, clang-tidy, gitleaks, dependency review, deterministic fuzz smoke, negative diagnostics and differential native/LLVM checks. Benchmarks are manual/advisory because shared CI runners are noisy. Release archives include SHA-256 sidecars; see [reproducible builds](docs/reproducible-builds.md).

