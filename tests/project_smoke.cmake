if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
set(SMOKE_DIR "${CMAKE_BINARY_DIR}/project-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")

execute_process(
  COMMAND "${NEXUS_BIN}" new "${SMOKE_DIR}" --template cli
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus new failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" info
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "version: 1[.]1[.]0")
  message(FATAL_ERROR "nexus info failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" assets
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "beep\\.wav" OR NOT out MATCHES "cube\\.obj")
  message(FATAL_ERROR "nexus assets failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" check
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "nexus check failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" build --release
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
set(EXPECTED_ARTIFACT "${SMOKE_DIR}/build/release/project-smoke")
if(WIN32)
  set(EXPECTED_ARTIFACT "${EXPECTED_ARTIFACT}.exe")
endif()
if(NOT rc EQUAL 0 OR NOT EXISTS "${EXPECTED_ARTIFACT}")
  message(FATAL_ERROR "nexus build failed: ${out}\n${err}")
endif()


execute_process(
  COMMAND "${NEXUS_BIN}" fmt
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus fmt failed: ${out}\n${err}")
endif()

# --check must fail for an unformatted file without modifying it.
set(FMT_CHECK_FILE "${SMOKE_DIR}/fmt-check.nx")
file(WRITE "${FMT_CHECK_FILE}" "fn main() { print(1) }   \n")
execute_process(
  COMMAND "${NEXUS_BIN}" fmt --check "${FMT_CHECK_FILE}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(rc EQUAL 0)
  message(FATAL_ERROR "nexus fmt --check should reject unformatted input")
endif()
file(READ "${FMT_CHECK_FILE}" fmt_content)
if(NOT fmt_content STREQUAL "fn main() { print(1) }   \n")
  message(FATAL_ERROR "nexus fmt --check modified source unexpectedly")
endif()
execute_process(COMMAND "${NEXUS_BIN}" fmt "${FMT_CHECK_FILE}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus fmt failed to format check fixture")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" fmt --check "${FMT_CHECK_FILE}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "fmt: OK")
  message(FATAL_ERROR "nexus fmt --check rejected formatted input: ${out}\\n${err}")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" lint
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "lint: OK")
  message(FATAL_ERROR "nexus lint failed: ${out}\n${err}")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" editor
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT EXISTS "${SMOKE_DIR}/.vscode/settings.json")
  message(FATAL_ERROR "nexus editor integration failed: ${out}\n${err}")
endif()
file(READ "${SMOKE_DIR}/.vscode/settings.json" editor_settings)
if(NOT editor_settings MATCHES "\\.nx.*nexus")
  message(FATAL_ERROR "nexus editor integration must associate .nx files with the NEXUS language ID: ${editor_settings}")
endif()
file(WRITE "${SMOKE_DIR}/src/main.nx" "fn main(){ const base: i64 = 2; let mut total: i64 = 0; for i in 1..6 { total += i * base } print(total) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "language v1 syntax smoke failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" check "${NEXUS_SOURCE_DIR}/examples/nexus_focus.nx"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "NEXUS Focus example failed semantic check: ${out}\\n${err}")
endif()

# Regression test for native execution when both the working directory and
# executable name contain spaces. This used to fail on Windows because run
# nested quoted paths inside cmd.exe /c.
set(RUN_SMOKE_DIR "${CMAKE_BINARY_DIR}/nexus run smoke")
file(REMOVE_RECURSE "${RUN_SMOKE_DIR}")
file(MAKE_DIRECTORY "${RUN_SMOKE_DIR}")
set(RUN_SOURCE "${RUN_SMOKE_DIR}/hello world.nx")
file(WRITE "${RUN_SOURCE}" "fn main() { print(\"NEXUS_RUN_OK\") }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" run "${RUN_SOURCE}"
  WORKING_DIRECTORY "${RUN_SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "NEXUS_RUN_OK")
  message(FATAL_ERROR "nexus run failed for a path containing spaces: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" "${RUN_SOURCE}"
  WORKING_DIRECTORY "${RUN_SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "NEXUS_RUN_OK")
  message(FATAL_ERROR "nexus <file.nx> shorthand failed: ${out}\n${err}")
endif()


# Verify local package addition/removal, lockfile freshness, and path traversal rejection.
set(PKG_SOURCE "${SMOKE_DIR}/package-source")
file(MAKE_DIRECTORY "${PKG_SOURCE}/src")
file(WRITE "${PKG_SOURCE}/nexus.toml" "[package]\nname = \"sample_lib\"\nversion = \"0.3.0\"\n\n[build]\nentry = \"src/main.nx\"\nartifact = \"sample_lib\"\ntarget = \"native\"\n")
file(WRITE "${PKG_SOURCE}/src/main.nx" "fn main() { print(\"sample\") }\n")
execute_process(COMMAND "${NEXUS_BIN}" pkg add "${PKG_SOURCE}" WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT EXISTS "${SMOKE_DIR}/vendor/sample_lib/src/main.nx")
  message(FATAL_ERROR "pkg add failed: ${out}\n${err}")
endif()
file(READ "${SMOKE_DIR}/nexus.toml" manifest_after_add)
file(READ "${SMOKE_DIR}/nexus.lock" lock_after_add)
if(NOT manifest_after_add MATCHES "vendor/sample_lib" OR NOT lock_after_add MATCHES "0.3.0"
   OR NOT lock_after_add MATCHES "\\[integrity\\]" OR NOT lock_after_add MATCHES "sha256:")
  message(FATAL_ERROR "pkg add failed to update dependency metadata/checksum")
endif()

execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "pkg verify: OK")
  message(FATAL_ERROR "pkg verify failed for an unchanged local package: ${out}\\n${err}")
endif()

file(WRITE "${SMOKE_DIR}/vendor/sample_lib/src/main.nx" "fn main() { print(\"tampered\") }\n")
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(rc EQUAL 0 OR NOT err MATCHES "integrity mismatch")
  message(FATAL_ERROR "pkg verify did not detect modified package content: ${out}\\n${err}")
endif()
configure_file("${PKG_SOURCE}/src/main.nx" "${SMOKE_DIR}/vendor/sample_lib/src/main.nx" COPYONLY)
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "pkg verify: OK")
  message(FATAL_ERROR "pkg verify failed after restoring package content: ${out}\\n${err}")
endif()
file(WRITE "${SMOKE_DIR}/outside-sentinel.txt" "keep")
execute_process(COMMAND "${NEXUS_BIN}" pkg remove ../outside-sentinel.txt WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(rc EQUAL 0 OR NOT EXISTS "${SMOKE_DIR}/outside-sentinel.txt")
  message(FATAL_ERROR "pkg remove accepted traversal or touched an outside path")
endif()
execute_process(COMMAND "${NEXUS_BIN}" pkg remove sample_lib WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR EXISTS "${SMOKE_DIR}/vendor/sample_lib")
  message(FATAL_ERROR "pkg remove failed: ${out}\n${err}")
endif()
file(READ "${SMOKE_DIR}/nexus.toml" manifest_after_remove)
file(READ "${SMOKE_DIR}/nexus.lock" lock_after_remove)
if(manifest_after_remove MATCHES "sample_lib" OR lock_after_remove MATCHES "sample_lib")
  message(FATAL_ERROR "pkg remove left stale dependency metadata")
endif()
file(REMOVE "${SMOKE_DIR}/outside-sentinel.txt")

file(REMOVE_RECURSE "${SMOKE_DIR}" "${RUN_SMOKE_DIR}")
message(STATUS "NEXUS project workflow smoke test: PASS")
