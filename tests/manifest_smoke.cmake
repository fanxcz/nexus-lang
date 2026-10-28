# Manifest syntax coverage: the project and package commands share one parser, so
# the accepted TOML subset must be identical for both. Arrays are accepted
# structurally and are not exposed through the scalar accessors yet.
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(SMOKE_DIR "${CMAKE_BINARY_DIR}/manifest-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")
execute_process(COMMAND "${NEXUS_BIN}" new "${SMOKE_DIR}" --template cli
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "could not prepare the manifest fixture: ${out}\n${err}")
endif()
set(MANIFEST "${SMOKE_DIR}/nexus.toml")

function(write_manifest content)
  file(WRITE "${MANIFEST}" "${content}")
endfunction()

function(expect_accept label)
  execute_process(COMMAND "${NEXUS_BIN}" check
    WORKING_DIRECTORY "${SMOKE_DIR}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "${label}: manifest was rejected: rc=${rc}: ${out}\n${err}")
  endif()
endfunction()

function(expect_reject label expected)
  execute_process(COMMAND "${NEXUS_BIN}" check
    WORKING_DIRECTORY "${SMOKE_DIR}"
    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  set(combined "${out}\n${err}")
  if(rc EQUAL 0)
    message(FATAL_ERROR "${label}: manifest was accepted but should be rejected")
  endif()
  if(NOT combined MATCHES "${expected}")
    message(FATAL_ERROR "${label}: expected '${expected}' in: ${combined}")
  endif()
endfunction()

# A single-line array is valid TOML and must not be mistaken for a broken entry.
write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"
authors = ["Ada Lovelace", "Grace Hopper"]

[build]
entry = "src/main.nx"
artifact = "manifest_fixture"
]==])
expect_accept("single-line array")

# Multi-line arrays with comments, trailing commas and nesting.
write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"
keywords = [
  "nexus",   # language
  "compiler",
]

[build]
entry = "src/main.nx"
artifact = "manifest_fixture"

[[tool.matrix]]
values = [
  [1, 2],
  [3, 4],
]
]==])
expect_accept("multi-line and nested arrays")

# Brackets and hashes inside strings are data, not syntax.
write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"
title = "a ] b # c"

[build]
entry = "src/main.nx"
artifact = "manifest_fixture"
]==])
expect_accept("brackets and hashes in strings")

# Repeated array-of-tables headers are legal; keys repeat between instances but
# not inside one instance.
write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"

[build]
entry = "src/main.nx"
artifact = "manifest_fixture"

[[bin]]
name = "one"

[[bin]]
name = "two"
]==])
expect_accept("repeated array-of-tables")

write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"

[build]
entry = "src/main.nx"
artifact = "manifest_fixture"

[[bin]]
name = "one"
name = "again"
]==])
expect_reject("duplicate key in one array-of-tables instance" "duplicate key 'name'")

# An array that never closes must name the line that started it.
write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"
keywords = [
  "nexus",

[build]
entry = "src/main.nx"
]==])
expect_reject("unterminated array" "unterminated array")

# Mixing a regular table with an array-of-tables of the same name is ambiguous.
write_manifest([==[
[package]
name = "manifest_fixture"
version = "1.0.0"

[build]
entry = "src/main.nx"

[[build]]
artifact = "manifest_fixture"
]==])
expect_reject("table mixed with array-of-tables" "duplicate table")

# Arrays are not exposed through scalar accessors: an array-valued key must not
# leak its raw syntax into user-visible output.
write_manifest([==[
[package]
name = ["zzz_not_a_name"]
version = "1.0.0"

[build]
entry = "src/main.nx"
artifact = "manifest_fixture"
]==])
execute_process(COMMAND "${NEXUS_BIN}" info
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus info rejected an array-valued key: ${out}\n${err}")
endif()
if(out MATCHES "zzz_not_a_name")
  message(FATAL_ERROR "array value leaked into scalar output: ${out}")
endif()

# The package commands use the same parser, so they must accept arrays too.
execute_process(COMMAND "${NEXUS_BIN}" pkg list
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus pkg list rejected an array-valued key: ${out}\n${err}")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}")
message(STATUS "NEXUS manifest syntax smoke test: PASS")
