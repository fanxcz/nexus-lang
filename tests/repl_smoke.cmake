if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN was not provided.")
endif()

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-smoke-input.txt")
set(repl_main_history "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-main-history.txt")
file(REMOVE "${repl_main_history}")
file(WRITE "${input_file}"
"2 + 2\n"
"(8 + 4) / 3\n"
"2.5 * 4\n"
"2 > 1\n"
"let mut value = 21\n"
"value * 2\n"
"value = value + 1\n"
"value\n"
"\"NEXUS_\" + \"REPL_OK\"\n"
"1 / 0\n"
":quit\n")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${repl_main_history}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  TIMEOUT 90
)

file(REMOVE "${input_file}" "${repl_main_history}")

if(NOT rc EQUAL 0)
  message(FATAL_ERROR "NEXUS default interactive shell exited with ${rc}: ${err}\n${out}")
endif()

if(NOT out MATCHES "4" OR NOT out MATCHES "10" OR
   NOT out MATCHES "true" OR NOT out MATCHES "42" OR
   NOT out MATCHES "22" OR NOT out MATCHES "NEXUS_REPL_OK" OR
   NOT err MATCHES "division by zero")
  message(FATAL_ERROR "NEXUS REPL did not evaluate arithmetic and retain bindings: ${out}\n${err}")
endif()

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-history-input.txt")
set(history_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-persistent-history.txt")
file(REMOVE "${history_file}")
file(WRITE "${input_file}"
"let mut total = 20\n"
"total = total + 2\n"
":history\n"
":quit\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${history_file}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 90)
if(NOT rc EQUAL 0 OR NOT out MATCHES "let mut total = 20" OR NOT out MATCHES "total = total")
  message(FATAL_ERROR "REPL history write failed: ${out}\n${err}")
endif()

file(WRITE "${input_file}" ":history\n:quit\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${history_file}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 90)
if(NOT rc EQUAL 0 OR NOT out MATCHES "let mut total = 20" OR NOT out MATCHES "total = total")
  message(FATAL_ERROR "REPL history was not restored by a fresh process: ${out}\n${err}")
endif()

file(WRITE "${input_file}" ":history-clear\n:history\n:quit\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${history_file}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 90)
file(SIZE "${history_file}" history_size)
file(REMOVE "${input_file}" "${history_file}")
if(NOT rc EQUAL 0 OR NOT out MATCHES "history cleared" OR NOT out MATCHES "history is empty" OR NOT history_size EQUAL 0)
  message(FATAL_ERROR "REPL history clear failed: ${out}\n${err}")
endif()

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-alias-input.txt")
file(WRITE "${input_file}" "6 * 7\n:quit\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-alias-history.txt"
          "${NEXUS_BIN}" repl
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


set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-brace-input.txt")
set(brace_history "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-brace-history.txt")
file(REMOVE "${brace_history}")
file(WRITE "${input_file}"
"fn brace_text() { print(\"BRACE_REPL_OK{\") }\n"
"brace_text()\n"
"fn block_comment_text() { /* { ignored */ print(\"BLOCK_COMMENT_REPL_OK\") }\n"
"block_comment_text()\n"
"fn line_comment_text() { // } ignored\n"
"  print(\"LINE_COMMENT_REPL_OK\")\n"
"}\n"
"line_comment_text()\n"
":quit\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${brace_history}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 90)
file(REMOVE "${input_file}" "${brace_history}")
if(NOT rc EQUAL 0 OR NOT out MATCHES "BRACE_REPL_OK" OR
   NOT out MATCHES "BLOCK_COMMENT_REPL_OK" OR NOT out MATCHES "LINE_COMMENT_REPL_OK")
  message(FATAL_ERROR "REPL multiline detection counted braces inside strings/comments: ${out}\\n${err}")
endif()

# Pure control-flow blocks that mutate bindings must remain part of the
# reconstructed session state without having to execute the block twice per input.
set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-if-state-input.txt")
set(history_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-if-state-history.txt")
file(REMOVE "${history_file}")
file(WRITE "${input_file}"
"let mut total = 1\n"
"if true { total = total + 41 }\n"
"total\n"
":quit\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${history_file}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 90)
file(REMOVE "${input_file}" "${history_file}")
if(NOT rc EQUAL 0 OR NOT out MATCHES "42" OR err MATCHES "will not persist")
  message(FATAL_ERROR "REPL did not preserve state from a pure if block: ${out}\n${err}")
endif()

set(input_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-while-state-input.txt")
set(history_file "${CMAKE_CURRENT_BINARY_DIR}/nexus-repl-while-state-history.txt")
file(REMOVE "${history_file}")
file(WRITE "${input_file}"
"let mut steps = 0\n"
"while steps < 3 { steps = steps + 1 }\n"
"steps\n"
":quit\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "NEXUS_HISTORY_FILE=${history_file}" "${NEXUS_BIN}"
  INPUT_FILE "${input_file}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 90)
file(REMOVE "${input_file}" "${history_file}")
if(NOT rc EQUAL 0 OR NOT out MATCHES "3" OR err MATCHES "will not persist")
  message(FATAL_ERROR "REPL did not preserve state from a pure while block: ${out}\n${err}")
endif()

message(STATUS "NEXUS interactive shell smoke test: PASS")
