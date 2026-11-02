# Reproducible Builds and Artifact Verification

This guide distinguishes integrity verification from a fully reproducible build. A SHA-256 checksum detects whether an artifact changed after publication; by itself it does not prove which source revision produced the binary. Bit-for-bit reproducibility additionally requires pinned compilers, linkers, dependencies, flags, timestamps and packaging inputs.

## Verify a published release asset

Download an archive and its adjacent `.sha256` file from the same GitHub Release. On Linux:

```sh
sha256sum -c NEXUS-vX.Y.Z-linux-x86_64.tar.gz.sha256
```

On macOS:

```sh
shasum -a 256 -c NEXUS-vX.Y.Z-macos-aarch64.tar.gz.sha256
```

On Windows PowerShell:

```powershell
(Get-FileHash .\NEXUS-vX.Y.Z-windows-x86_64.zip -Algorithm SHA256).Hash.ToLowerInvariant()
Get-Content .\NEXUS-vX.Y.Z-windows-x86_64.zip.sha256
```

Compare the resulting digest with the checksum published alongside the same release asset. Do not fetch the binary from one location and checksum from an unrelated mirror.

## Build from a known source revision

```sh
git clone https://github.com/fanxcz/nexus-lang.git
cd nexus-lang
git checkout <reviewed-commit-or-tag>
git status --short
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
git rev-parse HEAD > build/source-revision.txt
cmake --version > build/toolchain.txt
c++ --version >> build/toolchain.txt
sha256sum build/nexus > build/nexus.sha256
```

The LLVM backend may invoke `clang++` via `NEXUS_CXX`; record that exact executable and version too. Graphics/runtime availability depends on SDL2 and host libraries.

## Compare with a release artifact

A local rebuild is meaningfully comparable only when source revision, compiler family/version, target triple, CMake generator, build type, compile/link flags and relevant dependencies match the release job. The release workflow pins a Zig C++ toolchain for package builds; inspect `.github/workflows/release.yml` for the actual toolchain version and packaging steps.

A different hash does not automatically imply tampering: timestamps, archive metadata, linker IDs and build paths can make builds non-bit-reproducible. A matching hash is strong byte-for-byte equality but does not replace source review or a trusted build pipeline.

## Release evidence to retain

- Full Git commit SHA and tag.
- Compiler/toolchain versions and target platform.
- CMake configure options and build type.
- Dependency versions, including SDL2 where used.
- SHA-256 for each executable/archive and its checksum file.
- Relevant CI workflow run and CTest results.

Never place credentials, private build logs or signing keys in the manifest.
