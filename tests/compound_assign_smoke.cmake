# End-to-end test for compound assignment to struct fields, array elements,
# and pointer targets. These used to silently store only the right-hand side,
# so `p.count += 5` produced 5 instead of 15.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/compound-assign-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/compound_assign.nx")
set(EXE "${SCRATCH}/compound_assign.exe")

execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile compound_assign.nx:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "compound_assign.exe exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(expected "1\n2\nab\n10\n8\n10")
if(NOT run_out STREQUAL expected)
  message(FATAL_ERROR
    "Unexpected compound assignment output:\n--- actual ---\n${run_out}\n--- expected ---\n${expected}")
endif()

message(STATUS "NEXUS compound assignment smoke test: PASS")
