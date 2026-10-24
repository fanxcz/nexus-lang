if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
set(PAINT_DIR "${CMAKE_BINARY_DIR}/paint-smoke")
file(REMOVE_RECURSE "${PAINT_DIR}")

execute_process(
  COMMAND "${NEXUS_BIN}" new "${PAINT_DIR}" --template paint
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus new paint failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" check
  WORKING_DIRECTORY "${PAINT_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "nexus paint check failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" build --release
  WORKING_DIRECTORY "${PAINT_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
set(EXPECTED_PAINT_ARTIFACT "${PAINT_DIR}/build/release/paint-smoke")
if(WIN32)
  set(EXPECTED_PAINT_ARTIFACT "${EXPECTED_PAINT_ARTIFACT}.exe")
endif()
if(NOT rc EQUAL 0 OR NOT EXISTS "${EXPECTED_PAINT_ARTIFACT}")
  message(FATAL_ERROR "nexus paint build failed: ${out}\n${err}")
endif()
file(REMOVE_RECURSE "${PAINT_DIR}")
message(STATUS "NEXUS Paint project smoke test: PASS")
