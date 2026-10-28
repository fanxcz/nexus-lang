# NEXUS third-party dependency and license notices (v2.2.0).
#
# The NEXUS compiler, standard library sources, project templates, and the
# default runtime (src/runtime/) are original works of the NEXUS
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
#   They are not linked at build time: the runtime loads them dynamically
#   (dlopen/LoadLibrary). The official Windows portable package and app
#   installers bundle these runtime DLLs with accompanying license notices;
#   local source builds do not automatically bundle them. SDL components use
#   their own license terms; see https://raw.githubusercontent.com/libsdl-org/SDL/main/LICENSE.txt.
# - The official Windows portable package includes the Zig 0.14.1 toolchain
#   and a `clang++.cmd` compatibility wrapper to compile LLVM-backed programs
#   without a separate toolchain installation. Zig is a separate project
#   (MIT license, https://ziglang.org), not source-vendored into this repo;
#   the package includes Zig's license/readme. Redistribution must preserve
#   applicable notices and satisfy the toolchain's terms.
#
# Build tooling used to compile NEXUS itself (CMake, Ninja, platform C/C++
# compilers) is not distributed with NEXUS packages.
