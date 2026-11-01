# NEXUS toolchain selection.
#
# This file must be included before project() so CMake can initialize the
# platform rules for the selected C/C++ compiler. Keep compiler selection here;
# dependency discovery and target configuration happen after project().

# ============================================================================
# Compiler selection - ONLY CHANGE THESE IF NEEDED
# ============================================================================
# The compiler must be selected before project(). Forcing it afterwards leaves
# the platform rules of the default compiler in place, which on Windows mixes
# MinGW link flags with an MSVC-targeting clang and breaks the link step.
# Override via command line: -DNEXUS_C_COMPILER=clang -DNEXUS_CXX_COMPILER=clang++
# Or set in CMakePresets.json / CMakeUserPresets.json
option(NEXUS_USE_CLANG "Use Clang compiler (auto-detected on non-Windows)" ON)

# Allow manual override of compiler paths
set(NEXUS_C_COMPILER   "" CACHE STRING "Path to C compiler (clang/gcc/clang-cl)")
set(NEXUS_CXX_COMPILER "" CACHE STRING "Path to C++ compiler (clang++/g++/clang-cl)")
set(NEXUS_LINKER       "" CACHE STRING "Linker to use (lld/mold/gold/default)")

if (NEXUS_C_COMPILER)
  set(CMAKE_C_COMPILER ${NEXUS_C_COMPILER} CACHE STRING "C compiler" FORCE)
endif()
if (NEXUS_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER ${NEXUS_CXX_COMPILER} CACHE STRING "C++ compiler" FORCE)
endif()

# Auto-detect Clang on Windows if not explicitly set
if (WIN32 AND NEXUS_USE_CLANG AND NOT NEXUS_C_COMPILER)
  # Try common LLVM install locations
  set(LLVM_PATHS
    "C:/Program Files/LLVM"
    "C:/Program Files (x86)/LLVM"
    "C:/tools/llvm"
    "$ENV{USERPROFILE}/scoop/apps/llvm/current"
    "$ENV{LOCALAPPDATA}/Programs/LLVM"
  )
  foreach(path ${LLVM_PATHS})
    if (EXISTS "${path}/bin/clang.exe")
      set(CMAKE_C_COMPILER   "${path}/bin/clang.exe"   CACHE STRING "C compiler" FORCE)
      set(CMAKE_CXX_COMPILER "${path}/bin/clang++.exe" CACHE STRING "C++ compiler" FORCE)
      message(STATUS "Found LLVM at: ${path}")
      break()
    endif()
  endforeach()
endif()
