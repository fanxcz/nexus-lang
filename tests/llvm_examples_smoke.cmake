# Compile recently maintained interactive examples through LLVM codegen.
# They open real windows, so this smoke test compiles but does not run them.
if(NOT DEFINED NEXUS_BIN OR NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_BIN and NEXUS_SOURCE_DIR are required")
endif()

set(SCRATCH "\${CMAKE_BINARY_DIR}/llvm-example-smoke")
file(REMOVE_RECURSE "\${SCRATCH}")
file(MAKE_DIRECTORY "\${SCRATCH}")

foreach(EXAMPLE IN ITEMS "ui_playground.nx" "game2d_starter.nx")
  set(SOURCE "\${NEXUS_SOURCE_DIR}/examples/\${EXAMPLE}")
  get_filename_component(NAME "\${EXAMPLE}" NAME_WE)
  set(EXE "\${SCRATCH}/\${NAME}")
  execute_process(
    COMMAND "\${NEXUS_BIN}" build "\${SOURCE}" -o "\${EXE}" --backend llvm
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "LLVM compilation failed for \${EXAMPLE}:\\n\${out}\\n\${err}")
  endif()
  message(STATUS "PASS LLVM example \${EXAMPLE}")
endforeach()

message(STATUS "NEXUS interactive LLVM example smoke test: PASS")
