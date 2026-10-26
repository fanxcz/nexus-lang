# End-to-end smoke test for the NEXUS UI helper module.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/ui-library-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/ui_library_api.nx")
set(EXE "${SCRATCH}/ui_library_api")

execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile ui_library_api.nx:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "ui_library_api exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
if(NOT run_out STREQUAL "NEXUS UI library: PASS")
  message(FATAL_ERROR "Unexpected UI library output: [${run_out}]\n${run_err}")
endif()

message(STATUS "NEXUS UI library smoke test: PASS")
