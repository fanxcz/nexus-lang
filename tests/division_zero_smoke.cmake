if(NOT DEFINED NEXUS_BIN OR NOT DEFINED TEST_BINARY_DIR OR NOT DEFINED TEST_NATIVE_BACKEND)
  message(FATAL_ERROR "NEXUS_BIN, TEST_BINARY_DIR, and TEST_NATIVE_BACKEND are required")
endif()

set(TEST_DIR "${TEST_BINARY_DIR}/division-zero-smoke")
file(REMOVE_RECURSE "${TEST_DIR}")
file(MAKE_DIRECTORY "${TEST_DIR}")

if(WIN32)
  set(EXE_SUFFIX ".exe")
else()
  set(EXE_SUFFIX "")
endif()

set(BACKENDS llvm)
if(TEST_NATIVE_BACKEND)
  list(PREPEND BACKENDS native)
endif()

foreach(operation IN ITEMS divide remainder)
  if(operation STREQUAL "divide")
    set(expression "10 / denominator()")
  else()
    set(expression "10 % denominator()")
  endif()

  set(source "${TEST_DIR}/${operation}.nx")
  file(WRITE "${source}"
    "fn denominator() -> i64 { return 0 }\nfn main() { print(${expression}) }\n")

  foreach(backend IN LISTS BACKENDS)
    set(executable "${TEST_DIR}/${operation}-${backend}${EXE_SUFFIX}")
    execute_process(
      COMMAND "${NEXUS_BIN}" build "${source}" -o "${executable}" --backend "${backend}"
      RESULT_VARIABLE build_rc
      OUTPUT_VARIABLE build_out
      ERROR_VARIABLE build_err)
    if(NOT build_rc EQUAL 0)
      message(FATAL_ERROR
        "Could not compile dynamic ${operation} test using ${backend}:\n${build_out}\n${build_err}")
    endif()

    execute_process(
      COMMAND "${executable}"
      RESULT_VARIABLE run_rc
      OUTPUT_VARIABLE run_out
      ERROR_VARIABLE run_err)
    set(combined "${run_out}\n${run_err}")
    if(run_rc EQUAL 0 OR NOT combined MATCHES "NEXUS runtime error: division by zero")
      message(FATAL_ERROR
        "Expected a clean division-by-zero diagnostic using ${backend} for ${operation}; exit=${run_rc}\n${combined}")
    endif()
  endforeach()
endforeach()

file(REMOVE_RECURSE "${TEST_DIR}")
message(STATUS "NEXUS dynamic division-by-zero checks: PASS")
