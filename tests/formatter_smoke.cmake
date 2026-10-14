if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(TEST_DIR "${CMAKE_BINARY_DIR}/nexus-formatter-smoke")
file(MAKE_DIRECTORY "${TEST_DIR}")
set(TEST_SOURCE "${TEST_DIR}/formatter.nx")

file(WRITE "${TEST_SOURCE}" [=[fn main(){
let value:f64=1.25;
print("a  // b"); // note  spacing
}
]=])

execute_process(
  COMMAND "${NEXUS_BIN}" fmt "${TEST_SOURCE}"
  RESULT_VARIABLE format_result
  OUTPUT_VARIABLE format_output
  ERROR_VARIABLE format_error
)
if(NOT format_result EQUAL 0)
  message(FATAL_ERROR "nexus fmt failed (${format_result}): ${format_output}${format_error}")
endif()

file(READ "${TEST_SOURCE}" formatted)
set(expected [=[fn main() {
    let value: f64 = 1.25;
    print("a  // b"); // note  spacing
}
]=])
if(NOT formatted STREQUAL expected)
  message(FATAL_ERROR "Formatter produced unexpected output.\nExpected:\n${expected}\nActual:\n${formatted}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" check "${TEST_SOURCE}"
  RESULT_VARIABLE check_result
  OUTPUT_VARIABLE check_output
  ERROR_VARIABLE check_error
)
if(NOT check_result EQUAL 0)
  message(FATAL_ERROR "Formatted source no longer parses/type-checks (${check_result}): ${check_output}${check_error}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" fmt "${TEST_SOURCE}" --check
  RESULT_VARIABLE idempotency_result
  OUTPUT_VARIABLE idempotency_output
  ERROR_VARIABLE idempotency_error
)
if(NOT idempotency_result EQUAL 0)
  message(FATAL_ERROR "Formatter is not idempotent: ${idempotency_output}${idempotency_error}")
endif()

message(STATUS "Formatter preserves strings, comments and decimal literals; output parses and formatting is idempotent.")
