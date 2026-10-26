cmake_minimum_required(VERSION 3.20)
project(nexus VERSION 2.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Compiler selection - ONLY CHANGE THESE IF NEEDED
# ============================================================================
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
if (NEXUS_LINKER)
  set(CMAKE_LINKER ${NEXUS_LINKER} CACHE STRING "Linker" FORCE)
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
      # On Windows, use MSVC linker (link.exe) for compatibility
      # lld-link doesn't understand GNU flags like --out-implib
      if (EXISTS "${path}/bin/lld-link.exe")
        set(CMAKE_LINKER "${path}/bin/lld-link.exe" CACHE STRING "Linker" FORCE)
      elseif (EXISTS "${path}/bin/lld.exe")
        set(CMAKE_LINKER "${path}/bin/lld.exe" CACHE STRING "Linker" FORCE)
      endif()
      message(STATUS "Found LLVM at: ${path}")
      break()
    endif()
  endforeach()
endif()

# Prefer lld/mold if available (faster linking)
if (NOT NEXUS_LINKER)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    find_program(MOLD mold)
    if (MOLD)
      set(CMAKE_LINKER ${MOLD} CACHE STRING "Linker" FORCE)
      message(STATUS "Using mold linker: ${MOLD}")
    else()
      find_program(LLD lld lld-link)
      if (LLD)
        set(CMAKE_LINKER ${LLD} CACHE STRING "Linker" FORCE)
        message(STATUS "Using lld linker: ${LLD}")
      endif()
    endif()
  endif()
endif()

option(NEXUS_BUILD_TESTS "Build Nexus tests" ON)

# ============================================================================
# Find dependencies
# ============================================================================
# SDL2 - try vcpkg first, then pkg-config, then system paths
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(SDL2 QUIET sdl2)
  pkg_check_modules(SDL2_IMAGE QUIET SDL2_image)
  pkg_check_modules(SDL2_MIXER QUIET SDL2_mixer)
  pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
endif()

# If not found via pkg-config, try find_package (works with vcpkg)
if (NOT SDL2_FOUND)
  find_package(SDL2 QUIET)
  find_package(SDL2_image QUIET)
  find_package(SDL2_mixer QUIET)
  find_package(SDL2_ttf QUIET)
endif()

if (NOT SDL2_FOUND)
  message(WARNING "SDL2 not found! Graphics runtime will not work.")
  message(WARNING "Install SDL2 development libraries:")
  message(WARNING "  Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev")
  message(WARNING "  Arch: sudo pacman -S sdl2 sdl2_image sdl2_mixer sdl2_ttf")
  message(WARNING "  Fedora: sudo dnf install SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel")
  message(WARNING "  Windows (vcpkg): vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf --triplet x64-windows")
  message(WARNING "  Windows (choco): choco install sdl2")
  message(WARNING "  macOS: brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf")
endif()

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

# ============================================================================
# Sources
# ============================================================================
set(NEXUS_SOURCES
    src/cli/main.cpp
    src/compiler/format/formatter.cpp
    src/compiler/frontend/lexer/lexer.cpp
    src/compiler/frontend/parser/parser.cpp
    src/compiler/semantic/semantic.cpp
    src/compiler/backend/codegen/llvm_ir.cpp
    src/compiler/backend/native/native.cpp
    src/compiler/backend/native/target.cpp
    src/compiler/backend/native/pe_elf.cpp
    src/compiler/driver/driver.cpp
)

add_executable(nexus ${NEXUS_SOURCES})
# The official Windows portable package must run without an installed
# Visual C++ redistributable. Statically link the MSVC CRT into the compiler.
if (WIN32)
  # Applies to MSVC-ABI compilers (including compatible Clang builds).
  # Harmless for toolchains that use a different runtime model.
  set_property(TARGET nexus PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_include_directories(nexus PRIVATE
    src/compiler
    src/compiler/frontend
    src/compiler/backend)
target_compile_features(nexus PRIVATE cxx_std_20)
target_compile_definitions(nexus PRIVATE NEXUS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}" NEXUS_VERSION="${PROJECT_VERSION}")
target_compile_options(nexus PRIVATE ${NEXUS_COMPILE_OPTIONS})

# Link SDL2 if found
if (SDL2_FOUND)
  target_link_libraries(nexus PRIVATE ${SDL2_LIBRARIES})
  target_include_directories(nexus PRIVATE ${SDL2_INCLUDE_DIRS})
  if (SDL2_IMAGE_FOUND)
    target_link_libraries(nexus PRIVATE ${SDL2_IMAGE_LIBRARIES})
  endif()
  if (SDL2_MIXER_FOUND)
    target_link_libraries(nexus PRIVATE ${SDL2_MIXER_LIBRARIES})
  endif()
  if (SDL2_TTF_FOUND)
    target_link_libraries(nexus PRIVATE ${SDL2_TTF_LIBRARIES})
  endif()
endif()

# ============================================================================
# Fix linker flags for lld-link on Windows
# ============================================================================
if (WIN32 AND CMAKE_LINKER MATCHES "lld")
  # lld-link doesn't understand GNU flags like --out-implib
  # Use /OUT: instead and skip implib flags
  set(CMAKE_EXE_LINKER_FLAGS "")
  set(CMAKE_SHARED_LINKER_FLAGS "")
  set(CMAKE_STATIC_LINKER_FLAGS "")
  set(CMAKE_MODULE_LINKER_FLAGS "")
  # Tell CMake to use lld-link style flags
  set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
endif()

# ============================================================================
# Install
# ============================================================================
include(GNUInstallDirs)
target_compile_definitions(nexus PRIVATE NEXUS_INSTALL_RUNTIME_DIR="${CMAKE_INSTALL_FULL_DATADIR}/nexus/runtime")
install(TARGETS nexus RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES src/runtime/runtime.cpp DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/runtime)
install(DIRECTORY stdlib/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/stdlib OPTIONAL)
install(DIRECTORY examples/templates/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/project-templates OPTIONAL)
install(DIRECTORY examples/assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nexus/assets OPTIONAL)

# ============================================================================
# Tests
# ============================================================================
if (NEXUS_BUILD_TESTS)
  enable_testing()
  add_executable(nexus_tests
      tests/test_main.cpp
      src/compiler/format/formatter.cpp
      src/compiler/frontend/lexer/lexer.cpp
      src/compiler/frontend/parser/parser.cpp
      src/compiler/semantic/semantic.cpp
      src/compiler/backend/codegen/llvm_ir.cpp
      src/compiler/backend/native/native.cpp
      src/compiler/backend/native/target.cpp
      src/compiler/backend/native/pe_elf.cpp
      src/compiler/driver/driver.cpp
  )
  target_include_directories(nexus_tests PRIVATE
      src/compiler
      src/compiler/frontend
      src/compiler/backend)
  target_compile_features(nexus_tests PRIVATE cxx_std_20)
  target_compile_definitions(nexus_tests PRIVATE
      NEXUS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
      NEXUS_INSTALL_RUNTIME_DIR="${CMAKE_INSTALL_FULL_DATADIR}/nexus/runtime"
      NEXUS_VERSION="${PROJECT_VERSION}"
  )
  target_compile_options(nexus_tests PRIVATE ${NEXUS_COMPILE_OPTIONS})
  if (SDL2_FOUND)
    target_link_libraries(nexus_tests PRIVATE ${SDL2_LIBRARIES})
  endif()

  find_package(Threads REQUIRED)
  add_executable(nexus_runtime_memory_test
      tests/runtime_memory_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_memory_test PRIVATE cxx_std_20)
  target_compile_definitions(nexus_runtime_memory_test PRIVATE NEXUS_RUNTIME_TESTING)
  target_link_libraries(nexus_runtime_memory_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_memory_test COMMAND nexus_runtime_memory_test)

  add_executable(nexus_runtime_args_test
      tests/runtime_args_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_args_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_args_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_args_test
      COMMAND nexus_runtime_args_test alpha "beta gamma" omega)

  add_executable(nexus_runtime_json_test
      tests/runtime_json_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_json_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_json_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_json_test COMMAND nexus_runtime_json_test)

  add_executable(nexus_runtime_http_test
      tests/runtime_http_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_http_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_http_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_http_test COMMAND nexus_runtime_http_test)

  add_executable(nexus_frontend_fuzz_smoke
      tests/frontend_fuzz_smoke.cpp
      src/compiler/frontend/lexer/lexer.cpp
      src/compiler/frontend/parser/parser.cpp
  )
  target_include_directories(nexus_frontend_fuzz_smoke PRIVATE
      src/compiler
      src/compiler/frontend)
  target_compile_features(nexus_frontend_fuzz_smoke PRIVATE cxx_std_20)
  target_compile_options(nexus_frontend_fuzz_smoke PRIVATE ${NEXUS_COMPILE_OPTIONS})
  add_test(NAME nexus_frontend_fuzz_smoke COMMAND nexus_frontend_fuzz_smoke)
  set_tests_properties(nexus_frontend_fuzz_smoke PROPERTIES TIMEOUT 30)

  add_test(NAME nexus_tests COMMAND nexus_tests)
  add_test(NAME nexus_version_smoke COMMAND $<TARGET_FILE:nexus> --version)
  set_tests_properties(nexus_version_smoke PROPERTIES PASS_REGULAR_EXPRESSION "Nexus ${PROJECT_VERSION}")
  add_test(NAME nexus_repl_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DREPL_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/repl_smoke.cmake)
  add_test(NAME nexus_formatter_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/formatter_smoke.cmake)
  add_test(NAME nexus_self_test COMMAND $<TARGET_FILE:nexus> self-test)
  set_tests_properties(nexus_self_test PROPERTIES PASS_REGULAR_EXPRESSION "self-test: PASS")
  add_test(NAME nexus_doctor_json_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/doctor_json_smoke.cmake)

  add_test(NAME nexus_vscode_extension_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/vscode_extension_smoke.cmake)
  add_test(NAME nexus_backend_explain_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/backend_explain_smoke.cmake)
  add_test(NAME nexus_project_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/project_smoke.cmake)
  add_test(NAME nexus_examples_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/examples_smoke.cmake)
  add_test(NAME nexus_paint_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/paint_smoke.cmake)
  add_test(NAME nexus_diagnostics_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/diagnostics_smoke.cmake)
  add_test(NAME nexus_lsp_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/lsp_smoke.cmake)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if (Python3_Interpreter_FOUND)
    add_test(NAME nexus_lsp_watched_files_smoke
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/lsp_watched_files_smoke.py
            --nexus $<TARGET_FILE:nexus>)
    set_tests_properties(nexus_lsp_watched_files_smoke PROPERTIES TIMEOUT 45)
    if(UNIX)
      add_test(NAME nexus_repl_tty_smoke
          COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_SOURCE_DIR}/tests/repl_tty_smoke.py
              --nexus $<TARGET_FILE:nexus>)
      set_tests_properties(nexus_repl_tty_smoke PROPERTIES TIMEOUT 20)
    endif()
  endif()
  add_test(NAME nexus_regression_check
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/regression_check.cmake)
  add_test(NAME nexus_game2d_platformer_check
      COMMAND $<TARGET_FILE:nexus> check ${CMAKE_SOURCE_DIR}/examples/game2d_platformer.nx)
  set_tests_properties(nexus_game2d_platformer_check PROPERTIES
      PASS_REGULAR_EXPRESSION "check: OK")
  # The built-in executable image emitter is only runnable on x86-64
  # Linux and Windows. Keep its end-to-end executable tests off macOS.
  if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME nexus_native_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/native_smoke.cmake)
  endif()

  # Self-hosting: the NEXUS-written lexer's test suite and self-lexing smoke.
  # The .ll pipeline needs a clang-compatible driver, so plain g++ is skipped.
  find_program(NEXUS_SELFHOST_CXX NAMES clang++ clang++-18 clang++-17)
  if (NEXUS_SELFHOST_CXX)
    # The Paint project uses the LLVM backend, which needs a clang-compatible
    # driver. Reuse the driver found for the self-hosting tests.
    set_tests_properties(nexus_paint_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}")
    add_test(NAME nexus_selfhost_lexer
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/selfhost_lexer.cmake)
    set_tests_properties(nexus_selfhost_lexer PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}")
    add_test(NAME nexus_match_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/match_smoke.cmake)
    set_tests_properties(nexus_match_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}")
    add_test(NAME nexus_for_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/for_smoke.cmake)
    set_tests_properties(nexus_for_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}")
    add_test(NAME nexus_utf8_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/utf8_smoke.cmake)
    set_tests_properties(nexus_utf8_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}")
    add_test(NAME nexus_math_helpers_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/math_helpers_smoke.cmake)
    set_tests_properties(nexus_math_helpers_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}")
    add_test(NAME nexus_unicode_escape_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/unicode_escape_smoke.cmake)
    set_tests_properties(nexus_unicode_escape_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_tilemap_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/tilemap_smoke.cmake)
    set_tests_properties(nexus_tilemap_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_game_libraries_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/game_libraries_smoke.cmake)
    set_tests_properties(nexus_game_libraries_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_string_iteration_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/string_iteration_smoke.cmake)
    set_tests_properties(nexus_string_iteration_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)

    add_test(NAME nexus_defer_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/defer_smoke.cmake)
    set_tests_properties(nexus_defer_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)

    add_test(NAME nexus_checked_file_read_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/checked_file_read_smoke.cmake)
    set_tests_properties(nexus_checked_file_read_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)

    add_test(NAME nexus_collections_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/collections_smoke.cmake)
    set_tests_properties(nexus_collections_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_engine2d_api_smoke
        COMMAND $<TARGET_FILE:nexus> check ${CMAKE_SOURCE_DIR}/tests/regression_programs/engine2d_api.nx)
    set_tests_properties(nexus_engine2d_api_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "check: OK")
  endif()

  # The built-in emitters currently target x86-64 Linux and Windows.
  if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(NEXUS_PARITY_CXX NAMES clang++ clang++-18 g++ c++)
    if (NEXUS_SELFHOST_CXX)
      # Prefer the clang-compatible driver already used by the self-hosting
      # tests. A plain g++ cannot consume textual LLVM IR.
      set(NEXUS_PARITY_CXX ${NEXUS_SELFHOST_CXX})
    endif()
    if (NEXUS_PARITY_CXX)
      add_test(NAME nexus_backend_parity
          COMMAND ${CMAKE_COMMAND}
              -DNEXUS_BIN=$<TARGET_FILE:nexus>
              -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
              -P ${CMAKE_SOURCE_DIR}/tests/backend_parity.cmake)
      set_tests_properties(nexus_backend_parity PROPERTIES
          ENVIRONMENT "NEXUS_CXX=${NEXUS_PARITY_CXX}")
    endif()
  endif()
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "==========================================")
message(STATUS "NEXUS ${PROJECT_VERSION} Configuration Summary")
message(STATUS "==========================================")
message(STATUS "C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker:       ${CMAKE_LINKER}")
message(STATUS "Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2 Found:   ${SDL2_FOUND}")
if (SDL2_FOUND)
  message(STATUS "SDL2 Version: ${SDL2_VERSION}")
endif()
message(STATUS "Tests:        ${NEXUS_BUILD_TESTS}")
message(STATUS "==========================================")