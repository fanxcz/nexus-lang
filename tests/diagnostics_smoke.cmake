if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(DIAGNOSTIC_DIR "${CMAKE_BINARY_DIR}/diagnostics-smoke")
file(REMOVE_RECURSE "${DIAGNOSTIC_DIR}")
file(MAKE_DIRECTORY "${DIAGNOSTIC_DIR}")

set(BROKEN_SOURCE "${DIAGNOSTIC_DIR}/broken.nx")
file(WRITE "${BROKEN_SOURCE}" "fn main() {\n  let value = 1\n  print(value\n}\n")

execute_process(
  COMMAND "${NEXUS_BIN}" check "${BROKEN_SOURCE}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
set(combined "${out}\n${err}")
if(rc EQUAL 0)
  message(FATAL_ERROR "Malformed source unexpectedly passed check: ${combined}")
endif()
if(NOT combined MATCHES "broken.nx:4:1: error\\[NX1001\\]")
  message(FATAL_ERROR "Expected file/line/column syntax diagnostic, got: ${combined}")
endif()
string(FIND "${combined}" "4 | }" excerpt_index)
if(excerpt_index EQUAL -1 OR NOT combined MATCHES "\\^")
  message(FATAL_ERROR "Expected offending source line and caret, got: ${combined}")
endif()

file(REMOVE_RECURSE "${DIAGNOSTIC_DIR}")
message(STATUS "NEXUS diagnostics smoke test: PASS")
