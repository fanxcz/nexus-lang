# End-to-end test for UTF-8 string helpers.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/utf8-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/utf8_strings.nx")
set(EXE "${SCRATCH}/utf8_strings.exe")

execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile utf8_strings.nx:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "utf8_strings.exe exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(expected "12\n6\n1055\n1090\n-1\ntrue\n4\n1\n4\n128578\nA")
if(NOT run_out STREQUAL expected)
  message(FATAL_ERROR "Unexpected UTF-8 output:\n--- actual ---\n${run_out}\n--- expected ---\n${expected}")
endif()

message(STATUS "NEXUS UTF-8 string smoke test: PASS")
