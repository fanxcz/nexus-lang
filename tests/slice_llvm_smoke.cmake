if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR OR NOT DEFINED TEST_BINARY_DIR)
  message(FATAL_ERROR "NEXUS_BIN, NEXUS_SOURCE_DIR, and TEST_BINARY_DIR are required")
endif()

set(PROGRAM_DIR "${NEXUS_SOURCE_DIR}/tests/slice_programs")
set(SCRATCH "${TEST_BINARY_DIR}/slice-llvm-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")
if(WIN32)
  set(SUFFIX ".exe")
else()
  set(SUFFIX "")
endif()

set(EXE "${SCRATCH}/basic${SUFFIX}")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/basic.nx" -o "${EXE}" --backend llvm
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "LLVM build of basic slice program failed:\n${out}\n${err}")
endif()
execute_process(COMMAND "${EXE}" RESULT_VARIABLE rc OUTPUT_VARIABLE got ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "basic slice program failed with ${rc}: ${err}")
endif()
string(REPLACE "\r\n" "\n" got "${got}")
string(REGEX REPLACE "\n+$" "" got "${got}")
set(expected "4\n2\n10\n20\n30\n1\n20\n3")
if(NOT got STREQUAL expected)
  message(FATAL_ERROR "slice output mismatch:\n--- got ---\n${got}\n--- expected ---\n${expected}")
endif()

# The default auto selector must fall back to LLVM for slice types.
set(AUTO_EXE "${SCRATCH}/basic-auto${SUFFIX}")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/basic.nx" -o "${AUTO_EXE}" --backend auto
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "automatic backend selection failed for slice program:\n${out}\n${err}")
endif()
execute_process(COMMAND "${AUTO_EXE}" RESULT_VARIABLE rc OUTPUT_VARIABLE auto_got ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "auto-selected slice executable failed with ${rc}: ${err}")
endif()
string(REPLACE "\r\n" "\n" auto_got "${auto_got}")
string(REGEX REPLACE "\n+$" "" auto_got "${auto_got}")
if(NOT auto_got STREQUAL expected)
  message(FATAL_ERROR "auto backend did not produce expected slice output: ${auto_got}")
endif()

# The explicit native backend must reject slices rather than emitting invalid code.
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/basic.nx" -o "${SCRATCH}/basic-native${SUFFIX}" --backend native
  RESULT_VARIABLE native_rc OUTPUT_VARIABLE native_out ERROR_VARIABLE native_err)
if(native_rc EQUAL 0)
  message(FATAL_ERROR "native backend unexpectedly accepted the slice program")
endif()
if(NOT "${native_out}${native_err}" MATCHES "outside the builtin native")
  message(FATAL_ERROR "native backend rejected slices without an actionable diagnostic: ${native_out}${native_err}")
endif()

foreach(case index_oob range_oob)
  set(EXE "${SCRATCH}/${case}${SUFFIX}")
  execute_process(
    COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/${case}.nx" -o "${EXE}" --backend llvm
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "LLVM build of ${case} failed:\n${out}\n${err}")
  endif()
  execute_process(COMMAND "${EXE}" RESULT_VARIABLE rc OUTPUT_VARIABLE got ERROR_VARIABLE err)
  if(rc EQUAL 0)
    message(FATAL_ERROR "${case} should fail its runtime slice bounds check")
  endif()
  if(case STREQUAL "index_oob" AND NOT err MATCHES "out of bounds")
    message(FATAL_ERROR "index_oob had unexpected diagnostic: ${err}")
  endif()
  if(case STREQUAL "range_oob" AND NOT err MATCHES "invalid slice range")
    message(FATAL_ERROR "range_oob had unexpected diagnostic: ${err}")
  endif()
endforeach()

message(STATUS "NEXUS LLVM slice lowering: PASS")
