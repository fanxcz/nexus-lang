if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/unicode-escape-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")
set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/unicode_escapes.nx")
set(EXE "${SCRATCH}/unicode_escapes")

execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile Unicode escape test:\n${build_out}\n${build_err}")
endif()

set(OUT_FILE "${SCRATCH}/unicode_escapes.out")
execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_FILE "${OUT_FILE}"
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "Unicode escape test exited ${run_rc}:\n${run_err}")
endif()
# Read the program's stdout as raw bytes. CMake's OUTPUT_VARIABLE decodes
# through the host code page on Windows, which corrupts UTF-8 such as the
# emoji below; HEX comparison checks the exact bytes the runtime emitted.
file(READ "${OUT_FILE}" output_hex HEX)
string(TOLOWER "${output_hex}" output_hex)
string(REPLACE "0d0a" "0a" output_hex "${output_hex}")
if(NOT output_hex STREQUAL "41f09f99825a0ac2a90a")
  message(FATAL_ERROR "Incorrect Unicode escape output (hex): [${output_hex}]\n${run_err}")
endif()
message(STATUS "NEXUS Unicode string escape smoke test: PASS")
