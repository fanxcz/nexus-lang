# Regression tests for the NEXUS frontend + LLVM IR backend.
# Runs `check` and `--emit-ir` only, so no external C++ toolchain is needed.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
if(NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_SOURCE_DIR is required")
endif()

set(PROGRAM_DIR "${NEXUS_SOURCE_DIR}/tests/regression_programs")
file(GLOB PROGRAMS "${PROGRAM_DIR}/*.nx")
if(NOT PROGRAMS)
  message(FATAL_ERROR "no regression programs found in ${PROGRAM_DIR}")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/regression-ir")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(COUNT 0)
foreach(PROGRAM ${PROGRAMS})
  get_filename_component(NAME "${PROGRAM}" NAME)

  execute_process(
    COMMAND "${NEXUS_BIN}" check "${PROGRAM}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "regression check failed for ${NAME}: ${out}\n${err}")
  endif()

  set(IR_OUT "${SCRATCH}/${NAME}.out")
  execute_process(
    COMMAND "${NEXUS_BIN}" build "${PROGRAM}" -o "${IR_OUT}" --emit-ir
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "regression IR emission failed for ${NAME}: ${out}\n${err}")
  endif()
  if(NOT out MATCHES "define .*@main" AND NOT out MATCHES "define .* @main")
    message(FATAL_ERROR "regression IR for ${NAME} does not define @main")
  endif()

  math(EXPR COUNT "${COUNT} + 1")
  message(STATUS "  PASS ${NAME}")
endforeach()

message(STATUS "NEXUS regression (check + IR) smoke test: PASS (${COUNT} programs)")
