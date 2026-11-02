if(NOT DEFINED NEXUS_BIN OR NOT DEFINED TEST_SOURCE_DIR OR NOT DEFINED TEST_BINARY_DIR)
  message(FATAL_ERROR "NEXUS_BIN, TEST_SOURCE_DIR and TEST_BINARY_DIR are required")
endif()

function(expect_failure label input expected_pattern)
  execute_process(
    COMMAND "${NEXUS_BIN}" check "${input}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  set(combined "${out}
${err}")
  if(NOT rc EQUAL 2)
    message(FATAL_ERROR "${label}: expected exit code 2, got '${rc}':
${combined}")
  endif()
  if(NOT combined MATCHES "${expected_pattern}")
    message(FATAL_ERROR "${label}: diagnostic did not match '${expected_pattern}':
${combined}")
  endif()
  message(STATUS "PASS negative: ${label}")
endfunction()

expect_failure("syntax error / NX1001"
  "${TEST_SOURCE_DIR}/syntax_error.nx" "error\\[NX1001\\]")
expect_failure("integer literal overflow"
  "${TEST_SOURCE_DIR}/integer_overflow.nx" "integer literal is out of range for 'i64'")
expect_failure("array length overflow"
  "${TEST_SOURCE_DIR}/array_length_overflow.nx" "array length out of range at")
expect_failure("circular import"
  "${TEST_SOURCE_DIR}/cycle-a.nx" "circular import detected")

set(MISSING_SOURCE "${TEST_BINARY_DIR}/negative-missing-file.nx")
file(REMOVE "${MISSING_SOURCE}")
expect_failure("missing source file" "${MISSING_SOURCE}" "cannot open")

expect_failure("directory passed as source" "${TEST_SOURCE_DIR}" "is a directory")
message(STATUS "NEXUS negative tests: PASS")
