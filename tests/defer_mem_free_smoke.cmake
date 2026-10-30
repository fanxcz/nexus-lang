# Verify defer releases a managed allocation before a returned handle is used.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/defer-mem-free-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/defer_mem_free.nx")
set(EXE "${SCRATCH}/defer-mem-free")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to build defer_mem_free.nx:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(run_rc EQUAL 0)
  message(FATAL_ERROR "A handle released by defer was unexpectedly readable:\n${run_out}")
endif()
if(NOT "${run_err}" MATCHES "mem_read range is outside a live mem_alloc block")
  message(FATAL_ERROR "Deferred release did not produce the expected checked-memory error:\n${run_err}")
endif()

message(STATUS "NEXUS defer + mem_free smoke test: PASS")
