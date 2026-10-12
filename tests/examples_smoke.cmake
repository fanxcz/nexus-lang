if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
if(NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_SOURCE_DIR is required")
endif()

# Check every standalone example and source template, not just examples
# explicitly used by existing tests. This prevents stale snippets from quietly
# remaining in the repository after compiler syntax/runtime API changes.
set(EXAMPLE_RELATIVE_PATHS
  examples/hello.nx
  examples/imports.nx
  examples/interactive_cli.nx
  examples/terminal_game.nx
  examples/nexus_arena.nx
  examples/nexus_focus.nx
  examples/nexus_paint.nx
  examples/native_app.nx
  examples/native_graphics_game.nx
  examples/native_3d.nx
  examples/model_demo.nx
  examples/particles_demo.nx
  examples/audio_demo.nx
)
foreach(RELATIVE_SOURCE IN LISTS EXAMPLE_RELATIVE_PATHS)
  set(SOURCE_FILE "${NEXUS_SOURCE_DIR}/${RELATIVE_SOURCE}")
  get_filename_component(SOURCE_NAME "${SOURCE_FILE}" NAME)
  execute_process(
    COMMAND "${NEXUS_BIN}" check "${SOURCE_FILE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "NEXUS example failed semantic check: ${SOURCE_FILE}\n${out}\n${err}")
  endif()
  message(STATUS "PASS example ${SOURCE_NAME}")
endforeach()

# Check every checked-in template source, including editor.nx (which is an
# example but is not accepted by nexus new --template).
file(GLOB TEMPLATE_SOURCES "${NEXUS_SOURCE_DIR}/examples/templates/*.nx")
list(SORT TEMPLATE_SOURCES)
foreach(SOURCE_FILE IN LISTS TEMPLATE_SOURCES)
  get_filename_component(SOURCE_NAME "${SOURCE_FILE}" NAME)
  execute_process(
    COMMAND "${NEXUS_BIN}" check "${SOURCE_FILE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "NEXUS template source failed semantic check: ${SOURCE_FILE}\n${out}\n${err}")
  endif()
  message(STATUS "PASS template source ${SOURCE_NAME}")
endforeach()

# Also validate project generation itself for every template the current CLI
# advertises. The editor source exists but is deliberately not a CLI template.
set(TEMPLATE_ROOT "${CMAKE_BINARY_DIR}/template-check-smoke")
file(REMOVE_RECURSE "${TEMPLATE_ROOT}")
file(MAKE_DIRECTORY "${TEMPLATE_ROOT}")
foreach(TEMPLATE_NAME IN ITEMS cli app game 3d paint server library)
  set(PROJECT_DIR "${TEMPLATE_ROOT}/${TEMPLATE_NAME}-project")
  execute_process(
    COMMAND "${NEXUS_BIN}" new "${PROJECT_DIR}" --template "${TEMPLATE_NAME}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "nexus new failed for template ${TEMPLATE_NAME}:\n${out}\n${err}")
  endif()
  execute_process(
    COMMAND "${NEXUS_BIN}" check
    WORKING_DIRECTORY "${PROJECT_DIR}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "Generated ${TEMPLATE_NAME} project failed semantic check:\n${out}\n${err}")
  endif()
  message(STATUS "PASS generated template ${TEMPLATE_NAME}")
endforeach()

message(STATUS "NEXUS examples and template smoke tests: PASS")
