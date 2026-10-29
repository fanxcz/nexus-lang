# End-to-end test for match ranges, bindings, and guards.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/match-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/match_patterns.nx")
set(EXE "${SCRATCH}/match_patterns.exe")

execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile match_patterns.nx:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "match_patterns.exe exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(expected "zero\nunit\ntens\nnegative\nlarge\nlow\nhigh\nyes\nno\nbig=12\n1\n0\n20")
if(NOT run_out STREQUAL expected)
  message(FATAL_ERROR "Unexpected match output:\n--- actual ---\n${run_out}\n--- expected ---\n${expected}")
endif()


# Guarded arms cannot prove exhaustive control flow: a guard may evaluate false.
set(GUARDED_BOOL "${SCRATCH}/guarded_bool_return.nx")
file(WRITE "${GUARDED_BOOL}" "fn guarded(flag: bool) -> i64 {\n match flag {\n true if flag => { return 1 }\n false => { return 0 }\n }\n}\nfn main() { print(guarded(true)) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${GUARDED_BOOL}"
  RESULT_VARIABLE guarded_rc
  OUTPUT_VARIABLE guarded_out
  ERROR_VARIABLE guarded_err
)
set(guarded_diagnostics "${guarded_out}\n${guarded_err}")
if(guarded_rc EQUAL 0 OR NOT guarded_diagnostics MATCHES "match on bool is not exhaustive")
  message(FATAL_ERROR "A guarded boolean arm was incorrectly counted as exhaustive:\n${guarded_diagnostics}")
endif()

message(STATUS "NEXUS match range/binding/guard smoke test: PASS")
