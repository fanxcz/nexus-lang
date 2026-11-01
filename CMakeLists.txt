cmake_minimum_required(VERSION 3.20)

include(cmake/NexusOptions.cmake)
project(nexus VERSION 2.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Select a fast linker through the compiler driver instead of overriding
# CMAKE_LINKER. Clang and GCC both understand -fuse-ld=<flavor>, which keeps
# CMake's platform link rules intact.
set(NEXUS_LINK_FLAG "")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  if (NEXUS_LINKER)
    set(NEXUS_LINK_FLAG "-fuse-ld=${NEXUS_LINKER}")
    message(STATUS "Using linker flavor: ${NEXUS_LINKER}")
  else()
    find_program(MOLD mold)
    find_program(LLD lld lld-link)
    if (MOLD)
      set(NEXUS_LINK_FLAG "-fuse-ld=mold")
      message(STATUS "Using mold linker")
    elseif (LLD)
      set(NEXUS_LINK_FLAG "-fuse-ld=lld")
      message(STATUS "Using lld linker")
    endif()
  endif()
endif()

option(NEXUS_BUILD_TESTS "Build Nexus tests" ON)

include(cmake/NexusDependencies.cmake)

# ============================================================================
# Compiler flags
# ============================================================================
if (MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(NEXUS_COMPILE_OPTIONS /W4 /permissive- /utf-8)
else()
  set(NEXUS_COMPILE_OPTIONS
    -Wall -Wextra -Wpedantic -Wconversion
    -Wno-misleading-indentation -Wno-shadow
    -Wno-unused-parameter
  )
endif()

include(cmake/NexusSources.cmake)

include(cmake/NexusTargets.cmake)

include(cmake/NexusInstall.cmake)

include(cmake/NexusTests.cmake)

# ============================================================================
# Summary
# ============================================================================
message(STATUS "==========================================")
message(STATUS "NEXUS ${PROJECT_VERSION} Configuration Summary")
message(STATUS "==========================================")
message(STATUS "C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker:       ${NEXUS_LINK_FLAG}")
message(STATUS "Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2 Found:   ${SDL2_FOUND}")
if (SDL2_FOUND)
  message(STATUS "SDL2 Version: ${SDL2_VERSION}")
endif()
message(STATUS "Tests:        ${NEXUS_BUILD_TESTS}")
message(STATUS "==========================================")