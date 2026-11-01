if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR OR
   NOT DEFINED TEST_BINARY_DIR)
  message(FATAL_ERROR "NEXUS_BIN, NEXUS_SOURCE_DIR and TEST_BINARY_DIR are required")
endif()

set(SMOKE_DIR "\${TEST_BINARY_DIR}/os-config-dir-smoke")
set(SOURCE "\${NEXUS_SOURCE_DIR}/tests/os_config_dir.nx")
set(PROGRAM "\${SMOKE_DIR}/os-config-dir-smoke\${CMAKE_EXECUTABLE_SUFFIX}")
set(CONFIG_ROOT "\${SMOKE_DIR}/config")

file(REMOVE_RECURSE "\${SMOKE_DIR}")
file(MAKE_DIRECTORY "\${CONFIG_ROOT}")

execute_process(
  COMMAND "\${NEXUS_BIN}" build "\${SOURCE}" -o "\${PROGRAM}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "could not build std.os config directory smoke:\n\${build_out}\n\${build_err}")
endif()

execute_process(
  COMMAND "\${CMAKE_COMMAND}" -E env
    "APPDATA=\${CONFIG_ROOT}"
    "LOCALAPPDATA=\${CONFIG_ROOT}"
    "XDG_CONFIG_HOME=\${CONFIG_ROOT}"
    "HOME=\${SMOKE_DIR}"
    "\${PROGRAM}"
  WORKING_DIRECTORY "\${SMOKE_DIR}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0 OR NOT run_out MATCHES "NEXUS config directory smoke: PASS")
  message(FATAL_ERROR "std.os config directory smoke failed:\n\${run_out}\n\${run_err}")
endif()

if(NOT EXISTS "\${CONFIG_ROOT}/NEXUS Runtime Smoke")
  message(FATAL_ERROR "config_dir did not create the isolated application folder")
endif()
if(EXISTS "\${SMOKE_DIR}/outside")
  message(FATAL_ERROR "config_dir created a path outside its application-name boundary")
endif()

file(REMOVE_RECURSE "\${SMOKE_DIR}")
message(STATUS "NEXUS std.os config directory smoke: PASS")
