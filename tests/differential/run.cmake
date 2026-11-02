if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(FIXTURE_DIR "${NEXUS_SOURCE_DIR}/tests/native_programs")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/differential-tests")
file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

if(WIN32)
  set(EXE_SUFFIX ".exe")
else()
  set(EXE_SUFFIX "")
endif()

foreach(program IN ITEMS arith loops short_circuit bitwise)
  set(source "${FIXTURE_DIR}/${program}.nx")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "Differential fixture is missing: ${source}")
  endif()

  set(outputs)
  set(codes)
  foreach(backend IN ITEMS native llvm)
    set(executable "${OUTPUT_DIR}/${program}-${backend}${EXE_SUFFIX}")
    execute_process(
      COMMAND "${NEXUS_BIN}" build "${source}" -o "${executable}" --backend "${backend}"
      RESULT_VARIABLE build_rc OUTPUT_VARIABLE build_out ERROR_VARIABLE build_err)
    if(NOT build_rc EQUAL 0)
      message(FATAL_ERROR "${program}.nx failed to build with ${backend} (exit ${build_rc}):
${build_out}
${build_err}")
    endif()

    execute_process(
      COMMAND "${executable}"
      RESULT_VARIABLE run_rc OUTPUT_VARIABLE run_out ERROR_VARIABLE run_err)
    if(NOT run_rc EQUAL 0)
      message(FATAL_ERROR "${program}.nx using ${backend} exited ${run_rc}:
${run_out}
${run_err}")
    endif()
    string(REPLACE "
" "
" run_out "${run_out}")
    string(REGEX REPLACE "
+$" "" run_out "${run_out}")
    list(APPEND outputs "${run_out}")
    list(APPEND codes "${run_rc}")
  endforeach()

  list(GET outputs 0 native_output)
  list(GET outputs 1 llvm_output)
  list(GET codes 0 native_code)
  list(GET codes 1 llvm_code)
  if(NOT native_code STREQUAL llvm_code OR NOT native_output STREQUAL llvm_output)
    message(FATAL_ERROR "Differential mismatch for ${program}.nx:
--- native ---
${native_output}
--- LLVM ---
${llvm_output}")
  endif()
  message(STATUS "PASS differential: ${program}.nx")
endforeach()

file(REMOVE_RECURSE "${OUTPUT_DIR}")
message(STATUS "NEXUS native/LLVM differential tests: PASS")
