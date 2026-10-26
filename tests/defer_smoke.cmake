# End-to-end tests for lexical defer cleanup on the LLVM backend.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/defer-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/defer.nx")
set(EXE "${SCRATCH}/defer-smoke")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to build defer.nx:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "defer smoke test exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(expected [=[inside
nested
return-cleanup
9
break-cleanup
iteration
iteration
captured
first]=])
if(NOT run_out STREQUAL expected)
  message(FATAL_ERROR "Unexpected defer output:\n--- got ---\n${run_out}\n--- expected ---\n${expected}\n${run_err}")
endif()

# Defer must reject non-void calls rather than discarding a result silently.
set(INVALID "${SCRATCH}/defer-invalid.nx")
file(WRITE "${INVALID}" "fn value() -> i64 { return 1 }\nfn main() { defer value() }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${INVALID}"
  RESULT_VARIABLE invalid_rc
  OUTPUT_VARIABLE invalid_out
  ERROR_VARIABLE invalid_err
)
if(invalid_rc EQUAL 0)
  message(FATAL_ERROR "defer unexpectedly accepted a non-void call")
endif()
if(NOT "${invalid_out}${invalid_err}" MATCHES "defer requires a call returning void")
  message(FATAL_ERROR "Non-void defer error was not actionable: ${invalid_out}\n${invalid_err}")
endif()

# The limited built-in x86-64 backend must fail clearly instead of silently ignoring defer.
execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${SCRATCH}/native-output" --backend native
  RESULT_VARIABLE native_rc
  OUTPUT_VARIABLE native_out
  ERROR_VARIABLE native_err
)
if(native_rc EQUAL 0)
  message(FATAL_ERROR "The native backend unexpectedly accepted defer")
endif()
if(NOT "${native_out}${native_err}" MATCHES "defer is outside the builtin native v0 subset")
  message(FATAL_ERROR "Native-backend defer limitation was not explicit: ${native_out}\n${native_err}")
endif()

message(STATUS "NEXUS defer cleanup smoke test: PASS")
