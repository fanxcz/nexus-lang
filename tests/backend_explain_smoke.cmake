if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" explain-backend "${NEXUS_SOURCE_DIR}/tests/native_programs/arith.nx" --target x86_64-linux
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "explain-backend failed for native-compatible program: ${out}\n${err}")
endif()
if(NOT out MATCHES "auto will select: builtin native")
  message(FATAL_ERROR "native-compatible source did not select the builtin backend:\n${out}")
endif()
if(NOT out MATCHES "no external compiler required")
  message(FATAL_ERROR "native report omitted its toolchain requirements:\n${out}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" explain-backend "${NEXUS_SOURCE_DIR}/tests/regression_programs/types.nx" --target x86_64-linux
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "explain-backend failed for LLVM-fallback program: ${out}\n${err}")
endif()
if(NOT out MATCHES "auto will select: LLVM fallback")
  message(FATAL_ERROR "unsupported program did not explain the LLVM fallback:\n${out}")
endif()
if(NOT out MATCHES "reason:")
  message(FATAL_ERROR "LLVM fallback report omitted the reason:\n${out}")
endif()

message(STATUS "NEXUS backend explanation smoke test: PASS")
