if(NOT DEFINED NEXUS_BIN OR NOT DEFINED TEST_BINARY_DIR)
  message(FATAL_ERROR "NEXUS_BIN and TEST_BINARY_DIR are required")
endif()

set(TEST_DIR "${TEST_BINARY_DIR}/pointer-lifetime-smoke")
file(REMOVE_RECURSE "${TEST_DIR}")
file(MAKE_DIRECTORY "${TEST_DIR}")

# A pointer may be copied through local aliases and used while its owner lives.
# A nested borrow of an outer binding must also remain valid.
set(VALID_SOURCE "${TEST_DIR}/valid.nx")
file(WRITE "${VALID_SOURCE}"
  "fn read_local() -> i64 {\n"
  "  let mut value = 42\n"
  "  let pointer = &value\n"
  "  let alias = pointer\n"
  "  { let nested = &value; let nested_alias = nested; print(*nested_alias) }\n"
  "  return *alias\n"
  "}\n"
  "fn main() { print(read_local()) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${VALID_SOURCE}"
  RESULT_VARIABLE valid_rc OUTPUT_VARIABLE valid_out ERROR_VARIABLE valid_err)
if(NOT valid_rc EQUAL 0)
  message(FATAL_ERROR "Valid in-scope pointer use was rejected:\n${valid_out}\n${valid_err}")
endif()

# A local borrow must not be returned, including through one or more aliases.
set(RETURN_SOURCE "${TEST_DIR}/return-local.nx")
file(WRITE "${RETURN_SOURCE}"
  "fn leak() -> *i64 {\n"
  "  let mut value = 9\n"
  "  let pointer = &value\n"
  "  let alias = pointer\n"
  "  return alias\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${RETURN_SOURCE}"
  RESULT_VARIABLE return_rc OUTPUT_VARIABLE return_out ERROR_VARIABLE return_err)
set(return_combined "${return_out}\n${return_err}")
if(return_rc EQUAL 0 OR
   NOT return_combined MATCHES "cannot return a pointer to local storage 'value'")
  message(FATAL_ERROR "Returning a pointer to local storage was not rejected:\n${return_combined}")
endif()

# A pointer from a nested block must not be assigned to an outer binding.
set(ESCAPE_SOURCE "${TEST_DIR}/nested-escape.nx")
file(WRITE "${ESCAPE_SOURCE}"
  "fn main() {\n"
  "  let mut outer = 0\n"
  "  let mut escaped = &outer\n"
  "  {\n"
  "    let mut inner = 1\n"
  "    let local_pointer = &inner\n"
  "    let local_alias = local_pointer\n"
  "    escaped = local_alias\n"
  "  }\n"
  "}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${ESCAPE_SOURCE}"
  RESULT_VARIABLE escape_rc OUTPUT_VARIABLE escape_out ERROR_VARIABLE escape_err)
set(escape_combined "${escape_out}\n${escape_err}")
if(escape_rc EQUAL 0 OR
   NOT escape_combined MATCHES "cannot store pointer to shorter-lived local 'inner' in 'escaped'")
  message(FATAL_ERROR "A nested pointer escape was not rejected:\n${escape_combined}")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
message(STATUS "NEXUS pointer lifetime checks: PASS")
