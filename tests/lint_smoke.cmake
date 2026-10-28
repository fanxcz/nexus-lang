# `nexus lint` must distinguish trailing whitespace from the '\r' that CRLF
# files leave at the end of every line, and must keep reporting real markers.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(LINT_DIR "${CMAKE_BINARY_DIR}/lint-smoke")
file(REMOVE_RECURSE "${LINT_DIR}")
file(MAKE_DIRECTORY "${LINT_DIR}")

function(run_lint name content_var expect_regex forbid_regex)
  set(path "${LINT_DIR}/${name}.nx")
  file(WRITE "${path}" "${${content_var}}")
  execute_process(
    COMMAND "${NEXUS_BIN}" lint "${path}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "lint ${name}: non-zero exit ${rc}: ${out}\n${err}")
  endif()
  if(NOT out MATCHES "${expect_regex}")
    message(FATAL_ERROR "lint ${name}: expected '${expect_regex}' in: ${out}\n${err}")
  endif()
  if(NOT forbid_regex STREQUAL "" AND out MATCHES "${forbid_regex}")
    message(FATAL_ERROR "lint ${name}: unexpected '${forbid_regex}' in: ${out}\n${err}")
  endif()
endfunction()

# file(WRITE) rewrites '\n' to the platform line ending, so the CRLF fixtures
# spell '\r\n' and arrive as '\r\r\n' on Windows. Lint must strip every '\r'.
set(LF_CLEAN "fn main() {\n    print(1)\n}\n")
set(LF_TRAILING "fn main() {\n    print(1)   \n}\n")
set(CRLF_CLEAN "fn main() {\r\n    print(1)\r\n}\r\n")
set(CRLF_TRAILING "fn main() {\r\n    print(1) \r\n}\r\n")
set(MARKER "fn main() {\n    // TODO: keep this marker\n    print(1)\n}\n")

run_lint("lf_clean" LF_CLEAN "lint: OK" "NX9001")
run_lint("lf_trailing" LF_TRAILING "NX9001" "")
run_lint("crlf_clean" CRLF_CLEAN "lint: OK" "NX9001")
run_lint("crlf_trailing" CRLF_TRAILING "NX9001" "")
run_lint("marker" MARKER "NX9002" "")

file(REMOVE_RECURSE "${LINT_DIR}")
message(STATUS "NEXUS lint smoke test: PASS")
