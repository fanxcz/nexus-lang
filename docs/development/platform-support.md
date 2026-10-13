# Platform support matrix

This page separates hosts that are built by CI from targets the built-in native backend can emit. A successful compiler build on a host does not imply that every runtime feature or output target works there.

## Host CI coverage

| Host | Compiler build | Automated tests | Built-in native backend |
|---|---|---|---|
| Windows x64 | Yes | Yes, including Windows executable smoke tests | Emits x86-64 Windows PE executables; supported language subset only |
| Linux x64 (Ubuntu CI) | Yes | Yes, including Linux executable smoke tests and backend parity | Emits x86-64 Linux ELF executables; supported language subset only |
| macOS CI host | Yes | Yes, including compiler smoke tests | No built-in macOS executable emitter |

The CI matrix describes the runner environments configured in `.github/workflows/build.yml`. It is not a promise that prebuilt installers are published for every host.

## Compilation backends

- **Built-in native backend:** intended for the documented NEXUS language subset and x86-64 Windows PE / Linux ELF output. It does not require a separate C++ compiler for programs that fit that subset. Programs outside it should fail with an explanation rather than silently changing semantics.
- **LLVM/Clang path:** covers a broader portion of the language but needs a compatible `clang++` toolchain (or configured `NEXUS_CXX` / bundled toolchain). Actual target availability depends on the host toolchain and linking environment.
- **macOS:** the compiler itself is CI-built and tested, but the built-in direct machine-code backend currently has no macOS emitter.

## Runtime dependencies and limitations

| Area | Current constraint |
|---|---|
| Graphics, audio, and graphics-related runtime APIs | SDL2 and its optional companion libraries must be found and linked for corresponding features. The build can otherwise complete with those features unavailable. |
| Windows GUI applications | Current downloadable Studio and Paint binaries are Windows x64 artifacts. Their dependency and packaging requirements are separate from the compiler's CI build matrix. |
| Android and WebAssembly | These are not currently documented as first-class, tested release targets. They need target-specific runtime/ABI work and dedicated CI before being advertised as supported. |
| Other CPU architectures | The built-in executable emitter is x86-64-specific. Do not infer ARM64 support from the existence of LLVM IR generation. |

## Release checklist for adding a target

A target should be advertised as supported only after it has all of the following:

1. A documented target triple, ABI and toolchain requirement.
2. Compiler and runtime build jobs on the target or a documented cross-compilation host.
3. A compile-and-run test for a small program, plus regression coverage for target-specific runtime behavior.
4. A clean packaging/install test with documented dependencies and checksums for published binaries.
5. A clear statement of which standard-library and graphics/audio APIs are supported.

See the [testing guide](testing.md), [build targets](../build/targets.md), [native backend](../tooling/native-backend.md), and [roadmap](roadmap.md).
