if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SOURCE "${NEXUS_SOURCE_DIR}/examples/task_manager.nx")
file(READ "${SOURCE}" TASK_MANAGER_SOURCE)
string(FIND "${TASK_MANAGER_SOURCE}" "fn is_safe_command_token(" TOKEN_START)
string(FIND "${TASK_MANAGER_SOURCE}" "fn is_safe_pid(" PID_START)
string(FIND "${TASK_MANAGER_SOURCE}" "fn show_header(" HEADER_START)
if(TOKEN_START EQUAL -1 OR PID_START EQUAL -1 OR HEADER_START EQUAL -1 OR
   PID_START LESS TOKEN_START OR HEADER_START LESS PID_START)
  message(FATAL_ERROR "Could not extract the Task Manager input validators")
endif()

math(EXPR TOKEN_LENGTH "${PID_START} - ${TOKEN_START}")
math(EXPR PID_LENGTH "${HEADER_START} - ${PID_START}")
string(SUBSTRING "${TASK_MANAGER_SOURCE}" ${TOKEN_START} ${TOKEN_LENGTH} TOKEN_FUNCTION)
string(SUBSTRING "${TASK_MANAGER_SOURCE}" ${PID_START} ${PID_LENGTH} PID_FUNCTION)

set(SCRATCH "${CMAKE_BINARY_DIR}/task-manager-security-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")
set(PROGRAM "${SCRATCH}/validation.nx")
file(WRITE "${PROGRAM}" "${TOKEN_FUNCTION}\n${PID_FUNCTION}\n
fn main() {
    let mut passed = true
    if !is_safe_command_token(\"notepad.exe\") { passed = false }
    if !is_safe_command_token(\"calc\") { passed = false }
    if is_safe_command_token(\"..\") { passed = false }
    if is_safe_command_token(\"bad name.exe\") { passed = false }
    if is_safe_command_token(\"calc & whoami\") { passed = false }
    if is_safe_command_token(\"a|b\") { passed = false }
    if is_safe_command_token(\"C:/Windows/calc.exe\") { passed = false }
    if !is_safe_pid(\"1\") { passed = false }
    if !is_safe_pid(\"4294967295\") { passed = false }
    if is_safe_pid(\"0\") { passed = false }
    if is_safe_pid(\"0000\") { passed = false }
    if is_safe_pid(\"4294967296\") { passed = false }
    if is_safe_pid(\"12345678901\") { passed = false }
    if is_safe_pid(\"12&whoami\") { passed = false }
    if passed {
        print(\"NEXUS Task Manager security validation: PASS\")
    } else {
        print(\"NEXUS Task Manager security validation: FAIL\")
    }
}
")

set(EXE "${SCRATCH}/validation")
if(WIN32)
  set(EXE "${EXE}.exe")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" build "${PROGRAM}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0 OR NOT EXISTS "${EXE}")
  message(FATAL_ERROR "Task Manager validator build failed:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0 OR
   NOT run_out MATCHES "NEXUS Task Manager security validation: PASS" OR
   run_out MATCHES "NEXUS Task Manager security validation: FAIL")
  message(FATAL_ERROR "Task Manager input-validation tests failed:\n${run_out}\n${run_err}")
endif()
message(STATUS "NEXUS Task Manager input-validation smoke test: PASS")
