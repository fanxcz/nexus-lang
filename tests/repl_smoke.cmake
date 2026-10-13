if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN was not provided.")
endif()

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-smoke-input.txt")
file(WRITE "${input_file}"
"2 + 2\n"
"let value = 21\n"
"value * 2\n"
"value = value + 1\n"
"value\n"
"\"NEXUS_REPL_OK\"\n"
":quit\n")

execute_process(
  COMMAND "${NEXUS_BIN}"
  INPUT_FILE "${input_file}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  TIMEOUT 90
)

file(REMOVE "${input_file}")

if(NOT rc EQUAL 0)
  message(FATAL_ERROR "NEXUS default interactive shell exited with ${rc}: ${err}\n${out}")
endif()

if(NOT out MATCHES "4" OR NOT out MATCHES "42" OR
   NOT out MATCHES "22" OR NOT out MATCHES "NEXUS_REPL_OK")
  message(FATAL_ERROR "NEXUS REPL did not evaluate arithmetic and retain bindings: ${out}\n${err}")
endif()

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-alias-input.txt")
file(WRITE "${input_file}" "6 * 7\n:quit\n")
execute_process(
  COMMAND "${NEXUS_BIN}" repl
  INPUT_FILE "${input_file}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  TIMEOUT 90
)
file(REMOVE "${input_file}")

if(NOT rc EQUAL 0 OR NOT out MATCHES "42")
  message(FATAL_ERROR "The nexus repl command failed: ${err}\n${out}")
endif()

message(STATUS "NEXUS interactive shell smoke test: PASS")
