if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(DIAGNOSTIC_DIR "${CMAKE_BINARY_DIR}/diagnostics-smoke")
file(REMOVE_RECURSE "${DIAGNOSTIC_DIR}")
file(MAKE_DIRECTORY "${DIAGNOSTIC_DIR}")

set(BROKEN_SOURCE "${DIAGNOSTIC_DIR}/broken.nx")
file(WRITE "${BROKEN_SOURCE}" "fn main() {\n  let value = 1\n  print(value\n}\n")

execute_process(
  COMMAND "${NEXUS_BIN}" check "${BROKEN_SOURCE}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
set(combined "${out}\n${err}")
if(rc EQUAL 0)
  message(FATAL_ERROR "Malformed source unexpectedly passed check: ${combined}")
endif()
if(NOT combined MATCHES "broken.nx:4:1: error\\[NX1001\\]")
  message(FATAL_ERROR "Expected file/line/column syntax diagnostic, got: ${combined}")
endif()
string(FIND "${combined}" "4 | }" excerpt_index)
if(excerpt_index EQUAL -1 OR NOT combined MATCHES "\\^")
  message(FATAL_ERROR "Expected offending source line and caret, got: ${combined}")
endif()

# Independent semantic errors should be reported together and retain source ranges.
set(SEMANTIC_SOURCE "${DIAGNOSTIC_DIR}/semantic.nx")
file(WRITE "${SEMANTIC_SOURCE}" "fn main() {\n  print(missingOne)\n  print(missingTwo)\n}\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${SEMANTIC_SOURCE}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
set(semantic_combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT semantic_combined MATCHES "semantic.nx:2:9: error\\[NX2001\\]"
   OR NOT semantic_combined MATCHES "semantic.nx:3:9: error\\[NX2001\\]"
   OR NOT semantic_combined MATCHES "unknown variable: missingOne"
   OR NOT semantic_combined MATCHES "unknown variable: missingTwo")
  message(FATAL_ERROR "Expected two independent semantic diagnostics with source positions: ${semantic_combined}")
endif()

# Declaration validation must not mask independent errors later in the file.
set(DECLARATION_SOURCE "${DIAGNOSTIC_DIR}/declarations.nx")
file(WRITE "${DECLARATION_SOURCE}" "struct Broken { value: MissingType, value: i64 }\nfn main() { print(missingA); print(missingB) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${DECLARATION_SOURCE}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
set(declaration_combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT declaration_combined MATCHES "duplicate field 'value' in struct 'Broken'"
   OR NOT declaration_combined MATCHES "unknown type: MissingType"
   OR NOT declaration_combined MATCHES "unknown variable: missingA"
   OR NOT declaration_combined MATCHES "unknown variable: missingB")
  message(FATAL_ERROR "Semantic analysis did not aggregate declaration and body errors: ${declaration_combined}")
endif()

# Struct initializers must name every field exactly once.
set(DUPLICATE_INIT_SOURCE "${DIAGNOSTIC_DIR}/duplicate-init.nx")
file(WRITE "${DUPLICATE_INIT_SOURCE}"
  "struct Pair { left: i64, right: i64 }\nfn main() { let pair = Pair { left: 1, left: 2 } }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${DUPLICATE_INIT_SOURCE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(duplicate_init_combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT duplicate_init_combined MATCHES "duplicate field initializer 'left' in 'Pair'")
  message(FATAL_ERROR "Duplicate struct initializer field was not rejected: ${duplicate_init_combined}")
endif()

set(MISSING_INIT_SOURCE "${DIAGNOSTIC_DIR}/missing-init.nx")
file(WRITE "${MISSING_INIT_SOURCE}"
  "struct Pair { left: i64, right: i64 }\nfn main() { let pair = Pair { left: 1 } }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${MISSING_INIT_SOURCE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(missing_init_combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT missing_init_combined MATCHES "missing field 'right' in initializer for 'Pair'")
  message(FATAL_ERROR "Missing struct initializer field was not identified: ${missing_init_combined}")
endif()

set(UNKNOWN_INIT_SOURCE "${DIAGNOSTIC_DIR}/unknown-init.nx")
file(WRITE "${UNKNOWN_INIT_SOURCE}"
  "struct Pair { left: i64, right: i64 }\nfn main() { let pair = Pair { left: 1, height: 2 } }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${UNKNOWN_INIT_SOURCE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(unknown_init_combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT unknown_init_combined MATCHES "unknown field 'height' in initializer for 'Pair'")
  message(FATAL_ERROR "Unknown struct initializer field was not identified: ${unknown_init_combined}")
endif()

# Repeating a literal or enum case makes later match arms unreachable.
set(DUPLICATE_MATCH_SOURCE "${DIAGNOSTIC_DIR}/duplicate-match.nx")
file(WRITE "${DUPLICATE_MATCH_SOURCE}"
  "enum Color { Red, Green }\nfn main() { let color = Color::Red; match color { Color::Red => {}, Color::Red => {}, _ => {} } }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${DUPLICATE_MATCH_SOURCE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(duplicate_match_combined "${out}\n${err}")
if(rc EQUAL 0 OR NOT duplicate_match_combined MATCHES "duplicate match pattern 'Color::Red'")
  message(FATAL_ERROR "Duplicate enum match pattern was not rejected: ${duplicate_match_combined}")
endif()

# Recover from multiple malformed statements within one function body.
set(MULTI_SYNTAX_SOURCE "${DIAGNOSTIC_DIR}/syntax-multiple.nx")
file(WRITE "${MULTI_SYNTAX_SOURCE}"
  "fn main() { let = 1; print(2; let other = ; print(3) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check "${MULTI_SYNTAX_SOURCE}"
  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(multi_syntax_combined "${out}\n${err}")
string(REGEX MATCHALL "error\\[NX1001\\]" syntax_error_codes "${multi_syntax_combined}")
list(LENGTH syntax_error_codes syntax_error_count)
if(rc EQUAL 0 OR NOT syntax_error_count EQUAL 3)
  message(FATAL_ERROR "Parser did not recover all three syntax errors inside one function: ${multi_syntax_combined}")
endif()

file(REMOVE_RECURSE "${DIAGNOSTIC_DIR}")
message(STATUS "NEXUS diagnostics smoke test: PASS")
