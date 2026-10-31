if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/fixed-width-types-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NEXUS_SOURCE_DIR}/tests/regression_programs/fixed_width_types.nx")
set(EXE "${SCRATCH}/fixed_width_types")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${EXE}" --backend auto
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile fixed_width_types.nx with backend auto:\n${build_out}\n${build_err}")
endif()
# The direct machine-code backend must reject the same feature set explicitly.
execute_process(
  COMMAND "${NEXUS_BIN}" build "${SOURCE}" -o "${SCRATCH}/native_reject" --backend native
  RESULT_VARIABLE native_rc
  OUTPUT_VARIABLE native_out
  ERROR_VARIABLE native_err
)
set(native_combined "${native_out}\n${native_err}")
if(native_rc EQUAL 0 OR NOT native_combined MATCHES "outside the builtin native v0 subset")
  message(FATAL_ERROR "native backend did not reject fixed-width types explicitly:\n${native_combined}")
endif()

set(NARROW_SOURCE "${SCRATCH}/native_narrow.nx")
file(WRITE "${NARROW_SOURCE}" "fn main() { let value: i8 = 7; print(value) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${NARROW_SOURCE}" -o "${SCRATCH}/native_narrow" --backend native
  RESULT_VARIABLE narrow_rc
  OUTPUT_VARIABLE narrow_out
  ERROR_VARIABLE narrow_err
)
set(narrow_combined "${narrow_out}\n${narrow_err}")
if(narrow_rc EQUAL 0 OR NOT narrow_combined MATCHES "outside the builtin native v0 subset")
  message(FATAL_ERROR "native backend accepted i8 without other unsupported constructs:\n${narrow_combined}")
endif()

set(NARROW_AUTO_EXE "${SCRATCH}/native_narrow_auto")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${NARROW_SOURCE}" -o "${NARROW_AUTO_EXE}" --backend auto
  RESULT_VARIABLE narrow_auto_rc
  OUTPUT_VARIABLE narrow_auto_out
  ERROR_VARIABLE narrow_auto_err
)
if(NOT narrow_auto_rc EQUAL 0)
  message(FATAL_ERROR "backend auto did not fall back for i8:\n${narrow_auto_out}\n${narrow_auto_err}")
endif()
execute_process(
  COMMAND "${NARROW_AUTO_EXE}"
  RESULT_VARIABLE narrow_run_rc
  OUTPUT_VARIABLE narrow_run_out
  ERROR_VARIABLE narrow_run_err
)
string(STRIP "${narrow_run_out}" narrow_run_out)
if(NOT narrow_run_rc EQUAL 0 OR NOT narrow_run_out STREQUAL "7")
  message(FATAL_ERROR "backend auto produced an incorrect i8 result:\n${narrow_run_out}\n${narrow_run_err}")
endif()

set(FLOAT_SOURCE "${SCRATCH}/native_float.nx")
file(WRITE "${FLOAT_SOURCE}" "fn main() { let value: f32 = 1.5; print(value) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${FLOAT_SOURCE}" -o "${SCRATCH}/native_float" --backend native
  RESULT_VARIABLE float_rc
  OUTPUT_VARIABLE float_out
  ERROR_VARIABLE float_err
)
set(float_combined "${float_out}\n${float_err}")
if(float_rc EQUAL 0 OR NOT float_combined MATCHES "outside the builtin native v0 subset")
  message(FATAL_ERROR "native backend accepted f32 without other unsupported constructs:\n${float_combined}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "fixed_width_types exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(EXPECTED [=[-128
127
-32768
32767
-2147483648
2147483647
-9223372036854775808
255
65535
4294967295
18446744073709551615
3.5
5
30
2000000000
3.5
true
true
-3
1000000000
-4
64
15
-1234
4000000000
1.5
18446744073709551615
-12
34
1.75
30
-321
1.5
18446744073709551615
signed narrow range
unsigned wide range
narrow integer pattern]=])
if(NOT run_out STREQUAL EXPECTED)
  message(FATAL_ERROR "Unexpected fixed-width type output:\n[${run_out}]\nExpected:\n[${EXPECTED}]\n${run_err}")
endif()

# Invalid literals and implicit signedness conversions must fail semantic checks.
function(expect_rejected name source expected)
  set(path "${SCRATCH}/${name}.nx")
  file(WRITE "${path}" "${source}\n")
  execute_process(
    COMMAND "${NEXUS_BIN}" check "${path}"
    RESULT_VARIABLE check_rc
    OUTPUT_VARIABLE check_out
    ERROR_VARIABLE check_err
  )
  set(combined "${check_out}\n${check_err}")
  if(check_rc EQUAL 0 OR NOT combined MATCHES "${expected}")
    message(FATAL_ERROR "${name} was not rejected as expected:\n${combined}")
  endif()
endfunction()

expect_rejected(default_i64_overflow "fn main() { print(9223372036854775808) }" "integer literal is out of range for 'i64'")
expect_rejected(i8_overflow "fn main() { let value: i8 = 128 }" "cannot initialize 'value'")
expect_rejected(i8_underflow "fn main() { let value: i8 = -129 }" "cannot initialize 'value'")
expect_rejected(u8_overflow "fn main() { let value: u8 = 256 }" "cannot initialize 'value'")
expect_rejected(unsigned_negative "fn main() { let value: u8 = -1 }" "cannot initialize 'value'")
expect_rejected(implicit_width_conversion "fn main() { let wide: i64 = 1; let narrow: i8 = wide }" "cannot initialize 'narrow'")
expect_rejected(f32_overflow "fn main() { let value: f32 = 1e39 }" "cannot initialize 'value'")
expect_rejected(u64_overflow "fn main() { let value: u64 = 18446744073709551616 }" "integer literal out of range")

message(STATUS "NEXUS fixed-width numeric types smoke test: PASS")
