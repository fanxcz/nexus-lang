# Self-hosting step 1: run the NEXUS-written lexer's own test suite and the
# self-lexing smoke through the LLVM backend. Requires a clang-compatible
# NEXUS_CXX (the .ll path cannot be linked by plain g++).
if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
if(NOT DEFINED NEXUS_SOURCE_DIR)
  message(FATAL_ERROR "NEXUS_SOURCE_DIR is required")
endif()

set(SELFCASES
    "${NEXUS_SOURCE_DIR}/selfhost/lexer_test.nx"
    "${NEXUS_SOURCE_DIR}/selfhost/lex_self.nx"
    "${NEXUS_SOURCE_DIR}/selfhost/parser_test.nx"
    "${NEXUS_SOURCE_DIR}/selfhost/typecheck_test.nx")

foreach(CASE ${SELFCASES})
  get_filename_component(NAME "${CASE}" NAME)
  execute_process(
    COMMAND "${NEXUS_BIN}" run "${CASE}" --backend llvm
    WORKING_DIRECTORY "${NEXUS_SOURCE_DIR}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 300
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "selfhost lexer case ${NAME} failed (rc=${rc}):\n${out}\n${err}")
  endif()
  if(NOT out MATCHES "(all lexer tests passed|all parser tests passed|all typecheck tests passed|self-lex ok)")
    message(FATAL_ERROR "selfhost lexer case ${NAME} produced unexpected output:\n${out}\n${err}")
  endif()
endforeach()
message(STATUS "selfhost lexer tests passed")
