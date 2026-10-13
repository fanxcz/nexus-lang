# End-to-end tests for the builtin native backend (no external toolchain).
# Each program is compiled with --backend native, executed, and its stdout
# (and exit code) is compared against the expected text below.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
if(NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_SOURCE_DIR is required")
endif()

set(PROGRAM_DIR "${NEXUS_SOURCE_DIR}/tests/native_programs")
set(SCRATCH "${CMAKE_BINARY_DIR}/native-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

macro(check_native name expected)
  set(SRC "${PROGRAM_DIR}/${name}.nx")
  if(WIN32)
    set(EXE "${SCRATCH}/${name}.exe")
  else()
    set(EXE "${SCRATCH}/${name}")
  endif()
  execute_process(
    COMMAND "${NEXUS_BIN}" build "${SRC}" -o "${EXE}" --backend native
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "native build failed for ${name}: ${out}\n${err}")
  endif()
  if(NOT EXISTS "${EXE}")
    message(FATAL_ERROR "native build produced no executable for ${name}")
  endif()
  execute_process(
    COMMAND "${EXE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE got
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "native executable failed for ${name} (exit ${rc}): ${err}")
  endif()
  # Normalize trailing newlines for comparison.
  string(REGEX REPLACE "\r\n" "\n" got "${got}")
  string(REGEX REPLACE "\n+$" "" got "${got}")
  set(want "${expected}")
  string(REGEX REPLACE "\n+$" "" want "${want}")
  if(NOT got STREQUAL want)
    message(FATAL_ERROR "native output mismatch for ${name}:\n--- got ---\n${got}\n--- want ---\n${want}")
  endif()
  message(STATUS "  PASS ${name}.nx")
endmacro()

check_native(arith "14\n20\n42\n3\n2\n-3\n-2\n-5\n-42\ntrue\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\ntrue\nfalse\nfalse\ntrue")
check_native(calls "55\ntrue\nfalse\n21")
check_native(loops "64\n8\n5")
check_native(wide_frame "21\n210\n-9223372036854775807\n0")
check_native(print_types "literal line\ntrue\nfalse\ntrue\n42")
check_native(strings_full "\nabc\nHello, Nexus!\ntrue\nfalse\nfalse\ntrue\n-12345\n0\nanswer: 42")
check_native(string_edges "5\n72\n0\n0")

# input.nx reads one line from stdin; pipe a fixed name in.
if(WIN32)
  set(INPUT_EXE "${SCRATCH}/input.exe")
else()
  set(INPUT_EXE "${SCRATCH}/input")
endif()
file(WRITE "${SCRATCH}/input.txt" "Alice\n")
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/input.nx" -o "${INPUT_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for input")
endif()
execute_process(
  COMMAND "${INPUT_EXE}"
  INPUT_FILE "${SCRATCH}/input.txt"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE got
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native input executable failed (exit ${rc})")
endif()
string(REGEX REPLACE "\r\n" "\n" got "${got}")
if(NOT got STREQUAL "Your name: Hello, Alice!\ntrue\n")
  message(FATAL_ERROR "native input output mismatch:\n--- got ---\n${got}")
endif()
message(STATUS "  PASS input.nx")

# exit_code.nx prints nothing and returns 45 from main.
if(WIN32)
  set(EXIT_EXE "${SCRATCH}/exit_code.exe")
else()
  set(EXIT_EXE "${SCRATCH}/exit_code")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM_DIR}/exit_code.nx" -o "${EXIT_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for exit_code")
endif()
execute_process(COMMAND "${EXIT_EXE}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 45)
  message(FATAL_ERROR "native exit code mismatch: got ${rc}, want 45")
endif()
message(STATUS "  PASS exit_code.nx (exit 45)")

# Programs outside the subset must fail with an actionable error.
execute_process(
  COMMAND "${NEXUS_BIN}" build "${NEXUS_SOURCE_DIR}/tests/regression_programs/types.nx"
    -o "${SCRATCH}/rejected" --backend native
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(rc EQUAL 0)
  message(FATAL_ERROR "--backend native unexpectedly accepted types.nx")
endif()
if(NOT "${out}${err}" MATCHES "outside the builtin native v0 subset")
  message(FATAL_ERROR "--backend native rejection lacks an actionable reason: ${out}\n${err}")
endif()
message(STATUS "  PASS subset rejection is actionable")

message(STATUS "NEXUS builtin native backend smoke test: PASS")
