# WS1: corrupted project inputs must fail closed with an actionable message.
# None of these cases may be silently ignored, and none may crash the process.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(SMOKE_DIR "${CMAKE_BINARY_DIR}/corrupt-inputs-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")
execute_process(COMMAND "${NEXUS_BIN}" new "${SMOKE_DIR}" --template cli
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "could not prepare the corrupt-input fixture: ${out}\n${err}")
endif()
set(MANIFEST "${SMOKE_DIR}/nexus.toml")
file(READ "${MANIFEST}" valid_manifest)

function(expect_project_failure label expected)
  execute_process(COMMAND "${NEXUS_BIN}" check
    WORKING_DIRECTORY "${SMOKE_DIR}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  set(combined "${out}\n${err}")
  if(rc EQUAL 0 OR NOT combined MATCHES "${expected}")
    message(FATAL_ERROR
      "${label}: expected a non-zero exit with '${expected}', got rc=${rc}: ${combined}")
  endif()
endfunction()

# Truncated table header.
file(WRITE "${MANIFEST}" "[package\nname = \"broken\"\n")
expect_project_failure("truncated table header" "invalid table header")

# A key line without an assignment operator.
file(WRITE "${MANIFEST}" "[package]\nname = \"broken\"\n\n[build]\nentry\n")
expect_project_failure("missing assignment" "malformed entry")

# A repeated key would otherwise silently pick one of the values.
file(WRITE "${MANIFEST}" "[package]\nname = \"one\"\nname = \"two\"\n")
expect_project_failure("duplicate key" "duplicate key 'name'")

# A repeated table would otherwise merge two environments implicitly.
file(WRITE "${MANIFEST}" "[package]\nname = \"one\"\n\n[build]\nentry = \"src/main.nx\"\n\n[build]\nartifact = \"one\"\n")
expect_project_failure("duplicate table" "duplicate table \\[build\\]")

# Restore a valid manifest before testing source-level corruption.
file(WRITE "${MANIFEST}" "${valid_manifest}")

# Control bytes in a source file are rejected by the lexer with a location.
set(BAD_SOURCE "${SMOKE_DIR}/bad-control.nx")
string(ASCII 1 CONTROL_BYTE)
file(WRITE "${BAD_SOURCE}" "fn main() {")
file(APPEND "${BAD_SOURCE}" "${CONTROL_BYTE}")
file(APPEND "${BAD_SOURCE}" "}\n")
execute_process(COMMAND "${NEXUS_BIN}" check "${BAD_SOURCE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT combined MATCHES "unexpected character")
  message(FATAL_ERROR "control byte was not diagnosed: rc=${rc}: ${combined}")
endif()

# A manifest that points at a missing entry file must name the missing path.
file(WRITE "${MANIFEST}" "[package]\nname = \"corrupt_fixture\"\nversion = \"1.0.0\"\n\n[build]\nentry = \"src/missing.nx\"\nartifact = \"corrupt_fixture\"\n")
execute_process(COMMAND "${NEXUS_BIN}" build
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT combined MATCHES "cannot open" OR NOT combined MATCHES "missing\\.nx")
  message(FATAL_ERROR "missing entry file was not reported clearly: rc=${rc}: ${combined}")
endif()

# Passing a directory where a source file is expected must fail, not hang, and
# must not leak the C++ stream-layer message ("basic_filebuf::underflow"), which
# differs between libstdc++ and libc++.
execute_process(COMMAND "${NEXUS_BIN}" check "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT combined MATCHES "cannot open")
  message(FATAL_ERROR "directory source path was not rejected: rc=${rc}: ${combined}")
endif()
if(combined MATCHES "basic_filebuf|underflow")
  message(FATAL_ERROR "directory source path leaked an internal stream error: ${combined}")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}")
message(STATUS "NEXUS corrupt input smoke test: PASS")
