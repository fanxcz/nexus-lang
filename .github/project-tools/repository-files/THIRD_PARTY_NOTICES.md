# NEXUS third-party dependency and license notices (v1.1.0).
#
# The NEXUS compiler, standard library sources, project templates, and the
# default runtime (src/runtime/runtime.cpp) are original works of the NEXUS
# Project; see LICENSE at the repository root.
#
# No third-party compiler toolchain is bundled in this repository:
#
# - LLVM / Clang are NOT linked into `nexus` and are NOT shipped in NEXUS
#   packages. The optional LLVM backend shells out to a `clang++`-compatible
#   C++ compiler only when asked (full-language programs outside the builtin
#   native subset). Provide one via PATH, the NEXUS_CXX environment variable,
#   or a bundled wrapper; see `nexus doctor`.
# - The builtin native backend (x86-64 Windows PE / Linux ELF emitters) is
#   implemented from scratch in src/compiler/backend/native/ and has no
#   third-party code or data.
# - SDL2, SDL2_image, SDL2_mixer, and SDL2_ttf are OPTIONAL runtime libraries.
#   They are never linked at build time: the graphics/audio backend loads them
#   dynamically when present (dlopen/LoadLibrary) and otherwise reports a
#   clean runtime error. They are NOT shipped in NEXUS packages. If you install
#   them separately, their own licenses (zlib license) apply; see
#   https://raw.githubusercontent.com/libsdl-org/SDL/main/LICENSE.txt.
# - CI validates an optional self-contained configuration where a
#   Zig-provided `clang++.cmd` wrapper sits beside the NEXUS binary. Zig is a
#   separate project (MIT license, https://ziglang.org) and is NOT vendored in
#   this repository; operators who add it to a package must include its
#   license and respect its terms.
#
# Build tooling used to compile NEXUS itself (CMake, Ninja, platform C/C++
# compilers) is not distributed with NEXUS packages.
