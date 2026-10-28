# WS1: project workflows must work from directories and artifact names that
# contain spaces. Windows command quoting used to break here.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(SMOKE_ROOT "${CMAKE_BINARY_DIR}/project paths smoke")
file(REMOVE_RECURSE "${SMOKE_ROOT}")
file(MAKE_DIRECTORY "${SMOKE_ROOT}")

set(PROJECT_DIR "${SMOKE_ROOT}/My Project")
execute_process(
  COMMAND "${NEXUS_BIN}" new "${PROJECT_DIR}" --template cli
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT EXISTS "${PROJECT_DIR}/nexus.toml")
  message(FATAL_ERROR "nexus new failed for a path with spaces: ${out}\n${err}")
endif()

execute_process(COMMAND "${NEXUS_BIN}" check
  WORKING_DIRECTORY "${PROJECT_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "nexus check failed in a project path with spaces: ${out}\n${err}")
endif()

execute_process(COMMAND "${NEXUS_BIN}" fmt --all
  WORKING_DIRECTORY "${PROJECT_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus fmt --all failed in a project path with spaces: ${out}\n${err}")
endif()

execute_process(COMMAND "${NEXUS_BIN}" fmt --check --all
  WORKING_DIRECTORY "${PROJECT_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "fmt: OK")
  message(FATAL_ERROR "nexus fmt --check failed in a project path with spaces: ${out}\n${err}")
endif()

execute_process(COMMAND "${NEXUS_BIN}" lint
  WORKING_DIRECTORY "${PROJECT_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "lint: OK")
  message(FATAL_ERROR "nexus lint failed in a project path with spaces: ${out}\n${err}")
endif()

execute_process(COMMAND "${NEXUS_BIN}" build --release
  WORKING_DIRECTORY "${PROJECT_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(ARTIFACT "${PROJECT_DIR}/build/release/My Project")
if(WIN32)
  string(APPEND ARTIFACT ".exe")
endif()
if(NOT rc EQUAL 0 OR NOT EXISTS "${ARTIFACT}")
  message(FATAL_ERROR "nexus build failed with spaces in the project path: ${out}\n${err}")
endif()

execute_process(COMMAND "${ARTIFACT}"
  WORKING_DIRECTORY "${PROJECT_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "release artifact with spaces in its name failed to run: ${out}\n${err}")
endif()

# An explicit output path with spaces must be honoured too.
set(OUTPUT_DIR "${SMOKE_ROOT}/out dir")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(EXPLICIT_EXE "${OUTPUT_DIR}/My App")
if(WIN32)
  string(APPEND EXPLICIT_EXE ".exe")
endif()
execute_process(COMMAND "${NEXUS_BIN}" build "${PROJECT_DIR}/src/main.nx" -o "${EXPLICIT_EXE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT EXISTS "${EXPLICIT_EXE}")
  message(FATAL_ERROR "nexus build -o failed for an output path with spaces: ${out}\n${err}")
endif()
execute_process(COMMAND "${EXPLICIT_EXE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "artifact built to a path with spaces failed to run: ${out}\n${err}")
endif()

file(REMOVE_RECURSE "${SMOKE_ROOT}")
message(STATUS "NEXUS project path smoke test: PASS")
