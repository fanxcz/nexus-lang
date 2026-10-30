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
  "  if true { let nested = &value; let nested_alias = nested; print(*nested_alias) }\n"
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
  "  if true {\n"
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

# Branch-local assignments must be merged into the outer borrow state.
set(BRANCH_SOURCE "${TEST_DIR}/branch-escape.nx")
file(WRITE "${BRANCH_SOURCE}"
  "extern \"C\" fn external_pointer() -> *i64\n"
  "fn leak_branch() -> *i64 {\n"
  "  let mut local = 5\n"
  "  let mut pointer = external_pointer()\n"
  "  if true { pointer = &local }\n"
  "  return pointer\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${BRANCH_SOURCE}"
  RESULT_VARIABLE branch_rc OUTPUT_VARIABLE branch_out ERROR_VARIABLE branch_err)
set(branch_combined "${branch_out}\n${branch_err}")
if(branch_rc EQUAL 0 OR
   NOT branch_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "A branch-local borrow was lost during flow merging:\n${branch_combined}")
endif()

# Mutually exclusive if branches must not leak state into each other.
set(BRANCH_VALID_SOURCE "${TEST_DIR}/branch-valid.nx")
file(WRITE "${BRANCH_VALID_SOURCE}"
  "extern \"C\" fn external_pointer() -> *i64\n"
  "fn conditional_return() -> *i64 {\n"
  "  let mut local = 6\n"
  "  let mut pointer = external_pointer()\n"
  "  if true { pointer = &local } else { return pointer }\n"
  "  return external_pointer()\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${BRANCH_VALID_SOURCE}"
  RESULT_VARIABLE branch_valid_rc OUTPUT_VARIABLE branch_valid_out ERROR_VARIABLE branch_valid_err)
if(NOT branch_valid_rc EQUAL 0)
  message(FATAL_ERROR "Mutually exclusive branch state was merged incorrectly:\n${branch_valid_out}\n${branch_valid_err}")
endif()

# Pointer provenance must survive aggregate construction and reject local escapes.
set(AGGREGATE_SOURCE "${TEST_DIR}/aggregate-escape.nx")
file(WRITE "${AGGREGATE_SOURCE}"
  "struct PointerBox { value: *i64 }\n"
  "fn leak_box() -> PointerBox {\n"
  "  let mut local = 3\n"
  "  return PointerBox { value: &local }\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${AGGREGATE_SOURCE}"
  RESULT_VARIABLE aggregate_rc OUTPUT_VARIABLE aggregate_out ERROR_VARIABLE aggregate_err)
set(aggregate_combined "${aggregate_out}\n${aggregate_err}")
if(aggregate_rc EQUAL 0 OR
   NOT aggregate_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "Returning an aggregate with a local pointer was not rejected:\n${aggregate_combined}")
endif()

# Reading a pointer back out of an aggregate must retain its origin.
# Reading a pointer back out of an aggregate must retain its origin.
set(FIELD_SOURCE "${TEST_DIR}/field-escape.nx")
file(WRITE "${FIELD_SOURCE}"
  "struct PointerBox { value: *i64 }\n"
  "fn leak_field() -> *i64 {\n"
  "  let mut local = 8\n"
  "  let pointer_box = PointerBox { value: &local }\n"
  "  return pointer_box.value\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${FIELD_SOURCE}"
  RESULT_VARIABLE field_rc OUTPUT_VARIABLE field_out ERROR_VARIABLE field_err)
set(field_combined "${field_out}\n${field_err}")
if(field_rc EQUAL 0 OR
   NOT field_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "A pointer escape through a struct field was not rejected:\n${field_combined}")
endif()

# Array element extraction must preserve pointer provenance as well.
set(ARRAY_SOURCE "${TEST_DIR}/array-escape.nx")
file(WRITE "${ARRAY_SOURCE}"
  "fn leak_array() -> *i64 {\n"
  "  let mut local = 9\n"
  "  let pointers = [&local]\n"
  "  return pointers[0]\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${ARRAY_SOURCE}"
  RESULT_VARIABLE array_rc OUTPUT_VARIABLE array_out ERROR_VARIABLE array_err)
set(array_combined "${array_out}\n${array_err}")
if(array_rc EQUAL 0 OR
   NOT array_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "A pointer escape through an array element was not rejected:\n${array_combined}")
endif()

# Enum payload extraction must preserve pointer provenance too.
set(ENUM_SOURCE "${TEST_DIR}/enum-escape.nx")
file(WRITE "${ENUM_SOURCE}"
  "enum MaybePointer { Some(*i64), None }\n"
  "fn leak_enum() -> MaybePointer {\n"
  "  let mut local = 10\n"
  "  return MaybePointer::Some(&local)\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${ENUM_SOURCE}"
  RESULT_VARIABLE enum_rc OUTPUT_VARIABLE enum_out ERROR_VARIABLE enum_err)
set(enum_combined "${enum_out}\n${enum_err}")
if(enum_rc EQUAL 0 OR
   NOT enum_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "A pointer escape through an enum payload was not rejected:\n${enum_combined}")
endif()

set(MEMBER_SOURCE "${TEST_DIR}/member-escape.nx")
file(WRITE "${MEMBER_SOURCE}"
  "struct PointerBox { value: *i64 }\n"
  "fn main() {\n"
  "  let mut outer = 0\n"
  "  let mut box = PointerBox { value: &outer }\n"
  "  if true { let mut inner = 1; box.value = &inner }\n"
  "}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${MEMBER_SOURCE}"
  RESULT_VARIABLE member_rc OUTPUT_VARIABLE member_out ERROR_VARIABLE member_err)
set(member_combined "${member_out}\n${member_err}")
if(member_rc EQUAL 0 OR
   NOT member_combined MATCHES "cannot store pointer to shorter-lived local 'inner' in 'box'")
  message(FATAL_ERROR "A pointer escape through a struct field was not rejected:\n${member_combined}")
endif()

# A returned pointer is tied to the argument that supplied it, even through
# aliases, multiple forward-declared helpers, and a returned aggregate.
set(FORWARD_CALL_SOURCE "${TEST_DIR}/forward-call-escape.nx")
file(WRITE "${FORWARD_CALL_SOURCE}"
  "fn leak_forward() -> *i64 {\n"
  "  let mut local = 11\n"
  "  return relay(&local)\n"
  "}\n"
  "fn relay(pointer: *i64) -> *i64 { return identity(pointer) }\n"
  "fn identity(pointer: *i64) -> *i64 { let alias = pointer; return alias }\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${FORWARD_CALL_SOURCE}"
  RESULT_VARIABLE forward_call_rc OUTPUT_VARIABLE forward_call_out ERROR_VARIABLE forward_call_err)
set(forward_call_combined "${forward_call_out}\n${forward_call_err}")
if(forward_call_rc EQUAL 0 OR
   NOT forward_call_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "A local borrow escaped through pointer-returning helper calls:\n${forward_call_combined}")
endif()

set(AGGREGATE_CALL_SOURCE "${TEST_DIR}/aggregate-call-escape.nx")
file(WRITE "${AGGREGATE_CALL_SOURCE}"
  "struct PointerBox { value: *i64 }\n"
  "fn make_box(pointer: *i64) -> PointerBox { return PointerBox { value: pointer } }\n"
  "fn leak_box() -> *i64 {\n"
  "  let mut local = 12\n"
  "  let box = make_box(&local)\n"
  "  return box.value\n"
  "}\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${AGGREGATE_CALL_SOURCE}"
  RESULT_VARIABLE aggregate_call_rc OUTPUT_VARIABLE aggregate_call_out ERROR_VARIABLE aggregate_call_err)
set(aggregate_call_combined "${aggregate_call_out}\n${aggregate_call_err}")
if(aggregate_call_rc EQUAL 0 OR
   NOT aggregate_call_combined MATCHES "cannot return a pointer to local storage 'local'")
  message(FATAL_ERROR "A local borrow escaped through a returned aggregate:\n${aggregate_call_combined}")
endif()

# Returning an address of a value parameter is still a stack escape.
set(PARAMETER_ADDRESS_SOURCE "${TEST_DIR}/parameter-address-escape.nx")
file(WRITE "${PARAMETER_ADDRESS_SOURCE}"
  "fn leak_parameter(value: i64) -> *i64 { return &value }\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${PARAMETER_ADDRESS_SOURCE}"
  RESULT_VARIABLE parameter_address_rc OUTPUT_VARIABLE parameter_address_out ERROR_VARIABLE parameter_address_err)
set(parameter_address_combined "${parameter_address_out}\n${parameter_address_err}")
if(parameter_address_rc EQUAL 0 OR
   NOT parameter_address_combined MATCHES "cannot return a pointer to local storage 'value'")
  message(FATAL_ERROR "Taking the address of a value parameter escaped the function:\n${parameter_address_combined}")
endif()

# An opaque external pointer is not a local borrow. Passing it through a
# forwarding function must remain valid.
set(SAFE_CALL_SOURCE "${TEST_DIR}/safe-forward-call.nx")
file(WRITE "${SAFE_CALL_SOURCE}"
  "extern \"C\" fn external_pointer() -> *i64\n"
  "fn identity(pointer: *i64) -> *i64 { return pointer }\n"
  "fn safe_forward() -> *i64 { return identity(external_pointer()) }\n"
  "fn main() {}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${SAFE_CALL_SOURCE}"
  RESULT_VARIABLE safe_call_rc OUTPUT_VARIABLE safe_call_out ERROR_VARIABLE safe_call_err)
if(NOT safe_call_rc EQUAL 0)
  message(FATAL_ERROR "A pointer with opaque external provenance was rejected:\n${safe_call_out}\n${safe_call_err}")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
message(STATUS "NEXUS pointer lifetime checks: PASS")
