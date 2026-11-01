# Unit, integration, regression and platform-specific smoke tests.
# Included from the repository root so all source/binary paths remain stable.

# ============================================================================
# Tests
# ============================================================================
if (NEXUS_BUILD_TESTS)
  enable_testing()
  add_executable(nexus_tests
      tests/test_main.cpp
      src/compiler/format/formatter.cpp
      ${NEXUS_LEXER_SOURCES}
      ${NEXUS_PARSER_SOURCES}
      ${NEXUS_SEMANTIC_SOURCES}
      ${NEXUS_CODEGEN_SOURCES}
      ${NEXUS_NATIVE_SOURCES}
      ${NEXUS_DRIVER_SOURCES}
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
  
  add_executable(nexus_runtime_geometry_test
      tests/runtime_geometry_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_geometry_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_geometry_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_geometry_test COMMAND nexus_runtime_geometry_test)

  add_executable(nexus_runtime_numeric_test
      tests/runtime_numeric_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_numeric_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_numeric_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_numeric_test COMMAND nexus_runtime_numeric_test)

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

  add_executable(nexus_runtime_backend_state_test
      tests/runtime_backend_state_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_backend_state_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_backend_state_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_backend_state_test COMMAND nexus_runtime_backend_state_test)
  set_tests_properties(nexus_runtime_backend_state_test PROPERTIES TIMEOUT 30)

  add_executable(nexus_runtime_scene_test
      tests/runtime_scene_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(nexus_runtime_scene_test PRIVATE cxx_std_20)
  target_link_libraries(nexus_runtime_scene_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME nexus_runtime_scene_test COMMAND nexus_runtime_scene_test)
  set_tests_properties(nexus_runtime_scene_test PROPERTIES TIMEOUT 30)

  add_executable(nexus_frontend_fuzz_smoke
      tests/frontend_fuzz_smoke.cpp
      ${NEXUS_LEXER_SOURCES}
      ${NEXUS_PARSER_SOURCES}
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
  if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(NEXUS_NATIVE_DIVISION_TEST ON)
  else()
    set(NEXUS_NATIVE_DIVISION_TEST OFF)
  endif()
  add_test(NAME nexus_division_zero_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -DTEST_NATIVE_BACKEND=${NEXUS_NATIVE_DIVISION_TEST}
          -P ${CMAKE_SOURCE_DIR}/tests/division_zero_smoke.cmake)
  add_test(NAME nexus_pointer_lifetime_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/pointer_lifetime_smoke.cmake)
  add_test(NAME nexus_managed_allocation_lifetime_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/managed_allocation_lifetime_smoke.cmake)
  add_test(NAME nexus_doctor_json_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/doctor_json_smoke.cmake)
  add_test(NAME nexus_cli_arguments_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/cli_arguments_smoke.cmake)

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
  add_test(NAME nexus_project_paths_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/project_paths_smoke.cmake)
  add_test(NAME nexus_corrupt_inputs_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/corrupt_inputs_smoke.cmake)
  add_test(NAME nexus_templates_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/templates_smoke.cmake)
  add_test(NAME nexus_manifest_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/manifest_smoke.cmake)
  add_test(NAME nexus_lint_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNEXUS_BIN=$<TARGET_FILE:nexus>
          -P ${CMAKE_SOURCE_DIR}/tests/lint_smoke.cmake)
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
    add_test(NAME nexus_fetch_installer_smoke
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/fetch_installer_smoke.py)
    set_tests_properties(nexus_fetch_installer_smoke PROPERTIES TIMEOUT 120)
    add_test(NAME nexus_version_consistency
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/version_consistency_smoke.py
            --nexus $<TARGET_FILE:nexus>)
    set_tests_properties(nexus_version_consistency PROPERTIES TIMEOUT 60)
    add_test(NAME nexus_repository_sources
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/repository_sources_smoke.py
            --nexus $<TARGET_FILE:nexus>
            --root ${CMAKE_SOURCE_DIR})
    set_tests_properties(nexus_repository_sources PROPERTIES TIMEOUT 300)
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
  add_test(NAME nexus_game2d_starter_check
      COMMAND $<TARGET_FILE:nexus> check ${CMAKE_SOURCE_DIR}/examples/game2d_starter.nx)
  set_tests_properties(nexus_game2d_starter_check PROPERTIES
      PASS_REGULAR_EXPRESSION "check: OK")
  add_test(NAME nexus_ui_playground_check
      COMMAND $<TARGET_FILE:nexus> check ${CMAKE_SOURCE_DIR}/examples/ui_playground.nx)
  set_tests_properties(nexus_ui_playground_check PROPERTIES
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
    # The fixture reads a relative file, so the native backend must link the
    # runtime through the same clang-compatible driver as the self-host tests.
    add_test(NAME nexus_run_working_directory_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -P ${CMAKE_SOURCE_DIR}/tests/run_working_directory_smoke.cmake)
    set_tests_properties(nexus_run_working_directory_smoke PROPERTIES
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

    add_test(NAME nexus_compound_assign_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/compound_assign_smoke.cmake)
    set_tests_properties(nexus_compound_assign_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
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
    add_test(NAME nexus_math_trig_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/math_trig_smoke.cmake)
    set_tests_properties(nexus_math_trig_smoke PROPERTIES
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
    add_test(NAME nexus_game_primitives_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/game_primitives_smoke.cmake)
    set_tests_properties(nexus_game_primitives_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_enum_payloads_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/enum_payloads_smoke.cmake)
    set_tests_properties(nexus_enum_payloads_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_fixed_width_types_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/fixed_width_types_smoke.cmake)
    set_tests_properties(nexus_fixed_width_types_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 60)
    add_test(NAME nexus_slice_llvm_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/slice_llvm_smoke.cmake)
    set_tests_properties(nexus_slice_llvm_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 60)
    add_test(NAME nexus_mutable_slice_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/mutable_slice_smoke.cmake)
    set_tests_properties(nexus_mutable_slice_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 60)
    add_test(NAME nexus_stdlib_error_helpers_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/stdlib_error_helpers_smoke.cmake)
    set_tests_properties(nexus_stdlib_error_helpers_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_ui_library_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/ui_library_smoke.cmake)
    set_tests_properties(nexus_ui_library_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_app_game_llvm_examples_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/llvm_examples_smoke.cmake)
    set_tests_properties(nexus_app_game_llvm_examples_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 120)
    add_test(NAME nexus_task_manager_security_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/task_manager_security_smoke.cmake)
    set_tests_properties(nexus_task_manager_security_smoke PROPERTIES
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
    add_test(NAME nexus_defer_try_propagation_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/defer_try_propagation_smoke.cmake)
    set_tests_properties(nexus_defer_try_propagation_smoke PROPERTIES
        ENVIRONMENT "NEXUS_CXX=${NEXUS_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME nexus_defer_mem_free_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNEXUS_BIN=$<TARGET_FILE:nexus>
            -DNEXUS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/defer_mem_free_smoke.cmake)
    set_tests_properties(nexus_defer_mem_free_smoke PROPERTIES
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
