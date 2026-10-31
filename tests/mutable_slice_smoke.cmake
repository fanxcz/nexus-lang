if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR OR NOT DEFINED TEST_BINARY_DIR)
  message(FATAL_ERROR "NEXUS_BIN, NEXUS_SOURCE_DIR, and TEST_BINARY_DIR are required")
endif()

set(PROGRAM_DIR "${NEXUS_SOURCE_DIR}/tests/mutable_slice_programs")
set(SCRATCH "${TEST_BINARY_DIR}/mutable-slice-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")
if(WIN32)
  set(SUFFIX ".exe")
else()
  set(SUFFIX "")
endif()

function(build_and_check name expected backend)
  set(exe "${SCRATCH}/${name}-${backend}${SUFFIX}")
  execute_process(
    COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/${name}.nx" -o "${exe}" --backend "${backend}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "${backend} build of ${name} failed:\n${out}\n${err}")
  endif()
  execute_process(COMMAND "${exe}" RESULT_VARIABLE run_rc OUTPUT_VARIABLE got ERROR_VARIABLE run_err)
  if(NOT run_rc EQUAL 0)
    message(FATAL_ERROR "${name} executable failed (${run_rc}): ${run_err}")
  endif()
  string(REPLACE "\r\n" "\n" got "${got}")
  string(REGEX REPLACE "\n+$" "" got "${got}")
  if(NOT got STREQUAL expected)
    message(FATAL_ERROR "${name} output mismatch:\n--- got ---\n${got}\n--- expected ---\n${expected}")
  endif()
endfunction()

build_and_check(local "2\n20\n33\n40" llvm)
build_and_check(function "1\n70\n8" llvm)
build_and_check(local "2\n20\n33\n40" auto)

# Mutable slices are outside the native backend subset; auto must select LLVM.
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/function.nx" -o "${SCRATCH}/native${SUFFIX}" --backend native
  RESULT_VARIABLE native_rc OUTPUT_VARIABLE native_out ERROR_VARIABLE native_err)
if(native_rc EQUAL 0)
  message(FATAL_ERROR "native backend unexpectedly accepted mutable slices")
endif()
if(NOT "${native_out}${native_err}" MATCHES "outside the builtin native")
  message(FATAL_ERROR "native backend rejection was not actionable: ${native_out}${native_err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/index_oob.nx" -o "${SCRATCH}/index-oob${SUFFIX}" --backend llvm
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "mutable slice bounds test failed to compile:\n${out}\n${err}")
endif()
execute_process(COMMAND "${SCRATCH}/index-oob${SUFFIX}" RESULT_VARIABLE run_rc
                OUTPUT_VARIABLE run_out ERROR_VARIABLE run_err)
if(run_rc EQUAL 0 OR NOT run_err MATCHES "out of bounds")
  message(FATAL_ERROR "mutable slice index should fail its runtime bounds check: ${run_out}${run_err}")
endif()

function(expect_semantic_error name source expected)
  set(path "${SCRATCH}/${name}.nx")
  file(WRITE "${path}" "${source}\n")
  execute_process(COMMAND "${NEXUS_BIN}" check "${path}" RESULT_VARIABLE rc
                  OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc EQUAL 0)
    message(FATAL_ERROR "${name} unexpectedly passed semantic checking")
  endif()
  if(NOT "${out}${err}" MATCHES "${expected}")
    message(FATAL_ERROR "${name} diagnostic did not match '${expected}': ${out}${err}")
  endif()
endfunction()

expect_semantic_error(conflicting_access
  "fn main(){ let mut values:[i64;2]=[1,2]; let mut view:mut []i64=mut values[0..2]; print(values[0]); }"
  "cannot access 'values' while mutable slice 'view' is active")
expect_semantic_error(immutable_owner
  "fn main(){ let values:[i64;2]=[1,2]; let mut view:mut []i64=mut values[0..2]; }"
  "mutable slice requires an array declared with 'let mut'")
expect_semantic_error(overlapping_mutable_borrows
  "fn main(){ let mut values:[i64;3]=[1,2,3]; let mut a:mut []i64=mut values[0..2]; let mut b:mut []i64=mut values[1..3]; }"
  "cannot access 'values' while mutable slice 'a' is active")
expect_semantic_error(shared_and_mutable_borrow
  "fn main(){ let mut values:[i64;3]=[1,2,3]; let read=values[0..2]; let mut write:mut []i64=mut values[1..3]; }"
  "cannot create mutable slice while slice 'read' already borrows 'values'")
expect_semantic_error(read_reborrow_from_mutable_slice
  "fn main(){ let mut values:[i64;3]=[1,2,3]; let mut view:mut []i64=mut values[0..3]; let read=view[0..2]; }"
  "reborrowing a mutable slice is not supported")
expect_semantic_error(shared_and_mutable_call_arguments
  "fn update(write:mut []i64, read:[]i64){} fn main(){ let mut values:[i64;3]=[1,2,3]; update(mut values[0..2], values[1..3]); }"
  "conflicting shared and mutable slices of 'values' passed to one call")
expect_semantic_error(mutable_and_shared_call_arguments
  "fn update(read:[]i64, write:mut []i64){} fn main(){ let mut values:[i64;3]=[1,2,3]; update(values[1..3], mut values[0..2]); }"
  "conflicting shared and mutable slices of 'values' passed to one call")

message(STATUS "NEXUS mutable slice lowering and borrow checks: PASS")
