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

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "Unicode escape test exited ${run_rc}:\n${run_err}")
endif()
string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(HEX "${run_out}" output_hex)
string(TOLOWER "${output_hex}" output_hex)
if(NOT output_hex STREQUAL "41f09f99825a0ac2a90a")
  message(FATAL_ERROR "Incorrect Unicode escape output (hex): [${output_hex}]\n${run_err}")
endif()
message(STATUS "NEXUS Unicode string escape smoke test: PASS")
