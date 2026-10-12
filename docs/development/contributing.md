# Contributing to NEXUS

NEXUS is developed in the open. Useful contributions include compiler fixes, language and runtime tests, examples that compile, documentation corrections, packaging improvements, and reproducible bug reports.

The repository-level [CONTRIBUTING.md](https://github.com/fanxcz/nexus-lang/blob/main/CONTRIBUTING.md) is the canonical contributor guide. Also read the [Code of Conduct](https://github.com/fanxcz/nexus-lang/blob/main/CODE_OF_CONDUCT.md) and [Security Policy](https://github.com/fanxcz/nexus-lang/blob/main/SECURITY.md).

## 1. Fork and clone the correct repository

Fork `fanxcz/nexus-lang` on GitHub, then clone your fork:

```bash
git clone https://github.com/YOUR_USERNAME/nexus-lang.git
cd nexus-lang
git remote add upstream https://github.com/fanxcz/nexus-lang.git
```

Use `main` as the base branch. Do not target the unrelated `fanxcz/nexus` repository.

## 2. Build and run tests

CMake presets require CMake 3.25+ because the checked-in preset file uses schema version 6. A C++20 compiler is required; LLVM/Clang and SDL2 dependencies are needed for the paths and runtime features that use them.

```bash
cmake --preset=debug
cmake --build --preset=debug
ctest --test-dir build --output-on-failure
./build/nexus --version
./build/nexus self-test
```

On Windows, the compiler is normally `build/nexus.exe`. With CMake 3.20–3.24, configure explicitly rather than using presets:

```bash
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

For a language change, add a focused regression test under `tests/`. The test should demonstrate the old behavior and prevent regressions. For runtime and packaging changes, test on the affected platform when possible and describe what you could not test.

## 3. Make a focused branch

```bash
git checkout main
git pull upstream main
git checkout -b fix/short-description
```

Use prefixes such as `feat/`, `fix/`, `docs/`, `test/`, or `refactor/`. Keep a change small enough to review and keep generated files, build directories, executables and secrets out of unrelated commits.

## 4. Keep documentation honest

- Code samples should compile against the current compiler/runtime, or be clearly marked as pseudocode.
- Do not claim a backend or platform works until a real build and test demonstrates it.
- Explain backend limitations; a listed target is not necessarily supported by every backend.
- Update the relevant language, runtime, tooling, or template page when changing a public feature.
- For release and installer changes, verify the actual artifacts, checksums and links rather than relying only on a successful build step.

## 5. Test, commit and open a pull request

```bash
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
git status --short
git add <files>
git commit -m "fix: describe the change"
git push -u origin fix/short-description
```

Open a pull request against `fanxcz/nexus-lang:main`. Include the problem, what changed, tests run, and remaining limitations. Do not describe a test as passing unless it was actually run and passed.

## Where to contribute

- [Open issues](https://github.com/fanxcz/nexus-lang/issues)
- [GitHub Discussions](https://github.com/fanxcz/nexus-lang/discussions)
- [Root contributor guide](https://github.com/fanxcz/nexus-lang/blob/main/CONTRIBUTING.md)
- [Development roadmap](roadmap.md)
