if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

function(expect_cli_failure label expected)
  execute_process(
    COMMAND "${NEXUS_BIN}" ${ARGN}
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  set(combined "${out}\n${err}")
  if(rc EQUAL 0)
    message(FATAL_ERROR "${label}: expected failure, but command succeeded:\n${combined}")
  endif()
  if(NOT combined MATCHES "${expected}")
    message(FATAL_ERROR "${label}: expected diagnostic [${expected}], got:\n${combined}")
  endif()
endfunction()

execute_process(
  COMMAND "${NEXUS_BIN}" --help
  RESULT_VARIABLE help_rc
  OUTPUT_VARIABLE help_out
  ERROR_VARIABLE help_err
)
if(NOT help_rc EQUAL 0 OR NOT help_out MATCHES "Usage: nexus")
  message(FATAL_ERROR "nexus --help did not print usage:\n${help_out}\n${help_err}")
endif()

expect_cli_failure("zero benchmark runs" "bench --runs expects a positive integer"
  bench --runs 0)
expect_cli_failure("negative benchmark runs" "bench --runs expects a positive integer"
  bench --runs=-2)
expect_cli_failure("non-numeric benchmark runs" "bench --runs expects a positive integer"
  bench --runs=three)
expect_cli_failure("missing benchmark runs value" "bench --runs requires a value"
  bench --runs)
expect_cli_failure("unknown benchmark option" "unknown bench option"
  bench --not-a-real-option)
expect_cli_failure("multiple benchmark inputs" "bench accepts only one source file"
  bench first.nx second.nx)
expect_cli_failure("unknown lint option" "unknown lint option"
  lint --not-a-real-option)
expect_cli_failure("multiple lint inputs" "lint accepts only one source file"
  lint first.nx second.nx)
expect_cli_failure("doc unexpected input" "doc does not accept arguments"
  doc stray)

message(STATUS "NEXUS CLI argument validation: PASS")
