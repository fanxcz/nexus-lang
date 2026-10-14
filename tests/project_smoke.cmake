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

# Project-wide format and check are deterministic, recursive, and skip generated
# output and vendored dependencies.
set(FMT_ALL_FILE "${SMOKE_DIR}/src/fmt_all.nx")
set(FMT_ALL_NESTED "${SMOKE_DIR}/src/nested/fmt_nested.nx")
set(FMT_ALL_VENDOR "${SMOKE_DIR}/vendor/untouched.nx")
set(FMT_ALL_BUILD "${SMOKE_DIR}/build/untouched.nx")
file(MAKE_DIRECTORY "${SMOKE_DIR}/src/nested" "${SMOKE_DIR}/vendor" "${SMOKE_DIR}/build")
file(WRITE "${FMT_ALL_FILE}" "fn all(){print(1)}   \n")
file(WRITE "${FMT_ALL_NESTED}" "fn nested(){print(2)}\n")
file(WRITE "${FMT_ALL_VENDOR}" "fn vendor(){print(3)}   \n")
file(WRITE "${FMT_ALL_BUILD}" "fn build(){print(4)}   \n")
execute_process(COMMAND "${NEXUS_BIN}" fmt --check --all
  WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(rc EQUAL 0 OR NOT out MATCHES "fmt: needs formatting")
  message(FATAL_ERROR "fmt --check --all did not report unformatted project files: ${out}\n${err}")
endif()
file(READ "${FMT_ALL_FILE}" fmt_all_before)
if(NOT fmt_all_before STREQUAL "fn all(){print(1)}   \n")
  message(FATAL_ERROR "fmt --check --all modified source files unexpectedly")
endif()
execute_process(COMMAND "${NEXUS_BIN}" fmt --all
  WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "processed")
  message(FATAL_ERROR "fmt --all failed: ${out}\n${err}")
endif()
execute_process(COMMAND "${NEXUS_BIN}" fmt --check --all
  WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "fmt: OK")
  message(FATAL_ERROR "fmt --check --all did not pass after formatting: ${out}\n${err}")
endif()
file(READ "${FMT_ALL_FILE}" fmt_all_after)
file(READ "${FMT_ALL_NESTED}" fmt_nested_after)
if(NOT fmt_all_after STREQUAL "fn all() { print(1) }\n" OR
   NOT fmt_nested_after STREQUAL "fn nested() { print(2) }\n")
  message(FATAL_ERROR "fmt --all did not format root and nested source files")
endif()
file(READ "${FMT_ALL_VENDOR}" fmt_vendor_after)
file(READ "${FMT_ALL_BUILD}" fmt_build_after)
if(NOT fmt_vendor_after STREQUAL "fn vendor(){print(3)}   \n" OR
   NOT fmt_build_after STREQUAL "fn build(){print(4)}   \n")
  message(FATAL_ERROR "fmt --all modified vendored or generated files")
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

# Re-adding a package replaces its contents rather than leaving stale files behind.
file(WRITE "${SMOKE_DIR}/vendor/sample_lib/stale.txt" "stale")
execute_process(COMMAND "${NEXUS_BIN}" pkg add "${PKG_SOURCE}"
  WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR EXISTS "${SMOKE_DIR}/vendor/sample_lib/stale.txt")
  message(FATAL_ERROR "pkg add did not safely replace an existing package: ${out}\\n${err}")
endif()
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "pkg verify: OK")
  message(FATAL_ERROR "pkg verify failed after safely replacing a package: ${out}\\n${err}")
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

# The package bytes may still match, but a manifest edit must invalidate the lock.
file(READ "${SMOKE_DIR}/nexus.toml" manifest_locked)
string(REPLACE "vendor/sample_lib" "vendor/missing_lib" stale_manifest "${manifest_locked}")
file(WRITE "${SMOKE_DIR}/nexus.toml" "${stale_manifest}")
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(rc EQUAL 0 OR NOT err MATCHES "lockfile mismatch|missing local package")
  message(FATAL_ERROR "pkg verify did not reject stale dependency metadata: ${out}\\n${err}")
endif()
file(WRITE "${SMOKE_DIR}/nexus.toml" "${manifest_locked}")
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "pkg verify: OK")
  message(FATAL_ERROR "pkg verify failed after restoring the manifest: ${out}\\n${err}")
endif()

# Corrupt both sides consistently: an in-project path must still be rejected
# if it traverses above vendor/.
string(REPLACE "vendor/sample_lib" "vendor/../package-source" crafted_manifest "${manifest_locked}")
file(WRITE "${SMOKE_DIR}/nexus.toml" "${crafted_manifest}")
string(REPLACE "vendor/sample_lib" "vendor/../package-source" crafted_lock "${lock_after_add}")
file(WRITE "${SMOKE_DIR}/nexus.lock" "${crafted_lock}")
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(rc EQUAL 0 OR NOT err MATCHES "escapes vendor")
  message(FATAL_ERROR "pkg verify accepted a source path escaping vendor/: ${out}\\n${err}")
endif()
file(WRITE "${SMOKE_DIR}/nexus.toml" "${manifest_locked}")
file(WRITE "${SMOKE_DIR}/nexus.lock" "${lock_after_add}")
execute_process(COMMAND "${NEXUS_BIN}" pkg verify WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "pkg verify: OK")
  message(FATAL_ERROR "pkg verify failed after restoring dependency metadata: ${out}\\n${err}")
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

# Adding a local dependency must also work when the project has no lockfile yet.
set(NO_LOCK_DIR "${SMOKE_DIR}/without-lock")
file(MAKE_DIRECTORY "${NO_LOCK_DIR}/src")
file(WRITE "${NO_LOCK_DIR}/nexus.toml"
  "[package]\nname = \"no_lock_project\"\nversion = \"0.1.0\"\n\n[build]\nentry = \"src/main.nx\"\n")
file(WRITE "${NO_LOCK_DIR}/src/main.nx" "fn main() { print(1) }\n")
if(EXISTS "${NO_LOCK_DIR}/nexus.lock")
  message(FATAL_ERROR "no-lock test fixture unexpectedly has a lockfile")
endif()
execute_process(COMMAND "${NEXUS_BIN}" pkg add "${PKG_SOURCE}"
  WORKING_DIRECTORY "${NO_LOCK_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT EXISTS "${NO_LOCK_DIR}/nexus.lock")
  message(FATAL_ERROR "pkg add failed for a project without a lockfile: ${out}\\n${err}")
endif()
execute_process(COMMAND "${NEXUS_BIN}" pkg verify
  WORKING_DIRECTORY "${NO_LOCK_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "pkg verify: OK")
  message(FATAL_ERROR "new lockfile failed verification: ${out}\\n${err}")
endif()

file(REMOVE "${SMOKE_DIR}/outside-sentinel.txt")

# A symlinked vendor root must never redirect package writes outside the project.
if(NOT WIN32)
  file(REMOVE_RECURSE "${SMOKE_DIR}/vendor")
  set(OUTSIDE_VENDOR "${SMOKE_DIR}/outside-vendor")
  file(MAKE_DIRECTORY "${OUTSIDE_VENDOR}")
  file(WRITE "${OUTSIDE_VENDOR}/keep.txt" "keep")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${OUTSIDE_VENDOR}" "${SMOKE_DIR}/vendor"
    RESULT_VARIABLE symlink_rc OUTPUT_VARIABLE symlink_out ERROR_VARIABLE symlink_err)
  if(NOT symlink_rc EQUAL 0)
    message(FATAL_ERROR "could not prepare vendor symlink safety test: ${symlink_out}\\n${symlink_err}")
  endif()
  execute_process(COMMAND "${NEXUS_BIN}" pkg add "${PKG_SOURCE}"
    WORKING_DIRECTORY "${SMOKE_DIR}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(rc EQUAL 0 OR NOT err MATCHES "vendor directory must not be a symlink"
     OR NOT EXISTS "${OUTSIDE_VENDOR}/keep.txt")
    message(FATAL_ERROR "pkg add followed a symlinked vendor root: ${out}\\n${err}")
  endif()
  file(REMOVE "${SMOKE_DIR}/vendor")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}" "${RUN_SMOKE_DIR}")
message(STATUS "NEXUS project workflow smoke test: PASS")
