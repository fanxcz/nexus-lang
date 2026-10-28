# Every shipped project template must either scaffold a project that passes
# `nexus check`, or be rejected with an explicit "unknown template" error. The
# manifest validator added for WS1 must never reject a manifest that the
# compiler itself generated.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(TEMPLATE_DIR "${NEXUS_SOURCE_DIR}/examples/templates")
if(NOT IS_DIRECTORY "${TEMPLATE_DIR}")
  message(FATAL_ERROR "project templates are missing: ${TEMPLATE_DIR}")
endif()

set(SMOKE_DIR "${CMAKE_BINARY_DIR}/templates-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")
file(MAKE_DIRECTORY "${SMOKE_DIR}")

file(GLOB TEMPLATE_FILES "${TEMPLATE_DIR}/*.nx")
list(SORT TEMPLATE_FILES)
if(NOT TEMPLATE_FILES)
  message(FATAL_ERROR "no project templates found in ${TEMPLATE_DIR}")
endif()

set(supported 0)
foreach(TEMPLATE_FILE IN LISTS TEMPLATE_FILES)
  get_filename_component(TEMPLATE "${TEMPLATE_FILE}" NAME_WE)
  set(PROJECT_DIR "${SMOKE_DIR}/${TEMPLATE}")

  execute_process(
    COMMAND "${NEXUS_BIN}" new "${PROJECT_DIR}" --template "${TEMPLATE}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 0)
    set(combined "${out}\n${err}")
    if(combined MATCHES "unknown template: ${TEMPLATE}")
      message(STATUS "PASS template ${TEMPLATE} is explicitly unsupported")
    else()
      message(FATAL_ERROR "nexus new failed for template ${TEMPLATE}: ${combined}")
    endif()
    continue()
  endif()

  math(EXPR supported "${supported} + 1")
  if(NOT EXISTS "${PROJECT_DIR}/nexus.toml")
    message(FATAL_ERROR "template ${TEMPLATE} did not create a nexus.toml")
  endif()

  execute_process(
    COMMAND "${NEXUS_BIN}" check
    WORKING_DIRECTORY "${PROJECT_DIR}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR
      "a project generated from template ${TEMPLATE} failed nexus check: ${out}\n${err}")
  endif()
  message(STATUS "PASS template ${TEMPLATE}")
endforeach()

if(supported LESS 1)
  message(FATAL_ERROR "no project template could be scaffolded")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}")
message(STATUS "NEXUS template manifest smoke test: PASS (${supported} templates)")
