# `nexus run <file>` must start the program in the directory the command was
# invoked from. Building a file inside a project writes the executable under the
# project's build/ directory, so launching it from the executable's directory
# would silently break relative asset and data paths.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(SMOKE_DIR "${CMAKE_BINARY_DIR}/run-cwd-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")
file(MAKE_DIRECTORY "${SMOKE_DIR}/src")

file(WRITE "${SMOKE_DIR}/nexus.toml" [==[
[package]
name = "run_cwd_fixture"
version = "1.0.0"

[build]
entry = "src/main.nx"
artifact = "run_cwd_fixture"
]==])

file(WRITE "${SMOKE_DIR}/src/main.nx" [==[
import std.fs

fn main() {
    print(fs_read("marker.txt"))
}
]==])

file(WRITE "${SMOKE_DIR}/marker.txt" "NEXUS_RUN_CWD_OK")

execute_process(
  COMMAND "${NEXUS_BIN}" run src/main.nx
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "NEXUS_RUN_CWD_OK")
  message(FATAL_ERROR
    "nexus run did not start the program in the invocation directory: rc=${rc}\n${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" build src/main.nx
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus build rejected a file inside a project: rc=${rc}\n${out}\n${err}")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}")
message(STATUS "NEXUS run working-directory smoke test: PASS")
