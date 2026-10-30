# Check direct mem_alloc handles and local aliases for obvious use-after-free.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED TEST_BINARY_DIR)
  message(FATAL_ERROR "NEXUS_BIN and TEST_BINARY_DIR are required")
endif()

set(TEST_DIR "${TEST_BINARY_DIR}/managed-allocation-lifetime-smoke")
file(REMOVE_RECURSE "${TEST_DIR}")
file(MAKE_DIRECTORY "${TEST_DIR}")

function(check_source name source expected_result expected_diagnostic)
  set(source_path "${TEST_DIR}/${name}.nx")
  file(WRITE "${source_path}" "${source}")
  execute_process(
    COMMAND "${NEXUS_BIN}" check "${source_path}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  set(combined "${output}\n${error}")
  if(expected_result STREQUAL "pass")
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "${name} should pass semantic analysis:\n${combined}")
    endif()
  else()
    if(result EQUAL 0 OR NOT combined MATCHES "${expected_diagnostic}")
      message(FATAL_ERROR "${name} was not rejected as expected:\n${combined}")
    endif()
  endif()
endfunction()

check_source(valid_alias [=[
fn main() {
    let address = mem_alloc(16)
    let alias = address
    mem_set(alias, 1, 16)
    print(mem_read(address))
    mem_free(address)
}
]=] "pass" "")

check_source(use_after_free_alias [=[
fn main() {
    let address = mem_alloc(8)
    let alias = address
    mem_free(alias)
    let value = mem_read(address)
    print(value)
}
]=] "fail" "use after mem_free of managed allocation 'address' in mem_read")

check_source(double_free_alias [=[
fn main() {
    let address = mem_alloc(8)
    let alias = address
    mem_free(address)
    mem_free(alias)
}
]=] "fail" "double free of managed allocation 'alias'")

check_source(branch_free [=[
fn main() {
    let address = mem_alloc(8)
    let alias = address
    if true {
        mem_free(alias)
    }
    print(mem_read(address))
}
]=] "fail" "use after mem_free of managed allocation 'address' in mem_read")

check_source(copy_source_after_free [=[
fn main() {
    let address = mem_alloc(16)
    let alias = address
    mem_free(address)
    mem_copy(alias, 0, 8)
}
]=] "fail" "use after mem_free of managed allocation 'alias' in mem_copy")

check_source(deferred_free_is_not_eager [=[
fn main() {
    let address = mem_alloc(8)
    defer mem_free(address)
    mem_set(address, 7, 8)
    print(mem_read(address))
}
]=] "pass" "")

check_source(reassigned_owner_keeps_old_alias_invalid [=[
fn main() {
    let mut address = mem_alloc(8)
    let old_alias = address
    mem_free(address)
    address = mem_alloc(8)
    mem_set(address, 9, 8)
    print(mem_read(old_alias))
}
]=] "fail" "use after mem_free of managed allocation 'old_alias' in mem_read")

# Integers not derived from a local mem_alloc remain outside this narrow analysis.
check_source(untracked_integer [=[
fn main() {
    let address = 42
    print(address)
}
]=] "pass" "")

file(REMOVE_RECURSE "${TEST_DIR}")
message(STATUS "NEXUS managed allocation lifetime checks: PASS")
