if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(LSP_INPUT "${CMAKE_BINARY_DIR}/lsp-smoke.input")
set(init "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}")
set(open_bad "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn main() {\\n  print(1\\n}\\n\"}}}")
set(change_good "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\",\"version\":2},\"contentChanges\":[{\"text\":\"fn main() {\\n  print(42)\\n}\\n\"}]}}")
set(change_semantic "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\",\"version\":3},\"contentChanges\":[{\"text\":\"fn main() { print(unknown) }\"}]}}")
set(close_msg "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didClose\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\"}}}")
set(completion "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///hello.nx\"},\"position\":{\"line\":0,\"character\":0}}}")
set(open_definition "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn helper() {}\\nfn main() { helper() }\"}}}")
set(open_caller "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///caller.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn caller() { helper() }\"}}}")
set(open_hover "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn main() { print(42) }\"}}}")
set(definition "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14}}}")
set(formatting "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/formatting\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"options\":{\"tabSize\":4,\"insertSpaces\":true}}}")
set(references "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"textDocument/references\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14},\"context\":{\"includeDeclaration\":true}}}")
set(rename "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"textDocument/rename\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14},\"newName\":\"renamed_helper\"}}")
set(hover "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover.nx\"},\"position\":{\"line\":0,\"character\":13}}}")
set(shutdown "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"shutdown\",\"params\":null}")
set(exit_msg "{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":null}")

set(input_data "")
foreach(message IN ITEMS "${init}" "${open_bad}" "${change_good}" "${change_semantic}" "${close_msg}" "${completion}" "${open_definition}" "${open_caller}" "${open_hover}" "${definition}" "${formatting}" "${references}" "${rename}" "${hover}" "${shutdown}" "${exit_msg}")
  string(LENGTH "${message}" message_length)
  string(APPEND input_data "Content-Length: ${message_length}\r\n\r\n${message}")
endforeach()
file(WRITE "${LSP_INPUT}" "${input_data}")

execute_process(
  COMMAND "${NEXUS_BIN}" lsp
  INPUT_FILE "${LSP_INPUT}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "NEXUS LSP exited with ${rc}: ${err}")
endif()
if(NOT out MATCHES "completionProvider"
   OR NOT out MATCHES "\"label\":\"print\""
   OR NOT out MATCHES "\"definitionProvider\":true"
   OR NOT out MATCHES "\"uri\":\"file:///definition.nx\""
   OR NOT out MATCHES "\"line\":0,\"character\":3"
   OR NOT out MATCHES "textDocument/publishDiagnostics"
   OR NOT out MATCHES "NX1001"
   OR NOT out MATCHES "NX2001"
   OR NOT out MATCHES "unknown variable: unknown"
   OR NOT out MATCHES "\"character\":18"
   OR NOT out MATCHES "\"diagnostics\":\\[\\]")
  message(FATAL_ERROR "Expected initialize, completion, live error diagnostics, and diagnostic clearing, got: ${out}")
endif()

if(NOT out MATCHES "\\\"documentFormattingProvider\\\":true")
  message(FATAL_ERROR "NEXUS LSP did not advertise document formatting: ${out}")
endif()
string(FIND "${out}" "\"id\":5,\"result\":[{\"range\":" formatting_response_pos)
if(formatting_response_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP did not return a formatting TextEdit: ${out}")
endif()
string(FIND "${out}" "\"newText\":\"fn helper() {}\\nfn main() { helper() }\\n\"" formatting_text_pos)
if(formatting_text_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP formatting text differs from the shared formatter: ${out}")
endif()


if(NOT out MATCHES "referencesProvider" OR NOT out MATCHES "renameProvider")
  message(FATAL_ERROR "NEXUS LSP did not advertise references and rename: ${out}")
endif()
string(FIND "${out}" "\"id\":6,\"result\":[" references_start)
string(FIND "${out}" "\"id\":7,\"result\":{\"changes\":" rename_start)
if(references_start EQUAL -1 OR rename_start EQUAL -1 OR rename_start LESS references_start)
  message(FATAL_ERROR "NEXUS LSP did not return references and rename responses: ${out}")
endif()
math(EXPR references_length "${rename_start} - ${references_start}")
string(SUBSTRING "${out}" ${references_start} ${references_length} references_payload)
string(REGEX MATCHALL "\"uri\":\"file:///definition.nx\"" definition_locations "${references_payload}")
string(REGEX MATCHALL "\"uri\":\"file:///caller.nx\"" caller_locations "${references_payload}")
list(LENGTH definition_locations definition_location_count)
list(LENGTH caller_locations caller_location_count)
if(NOT definition_location_count EQUAL 2 OR NOT caller_location_count EQUAL 1)
  message(FATAL_ERROR "Expected 2 definition.nx and 1 caller.nx reference locations: ${references_payload}")
endif()
string(FIND "${out}" "\"newText\":\"renamed_helper\"" rename_text_pos)
if(rename_text_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP rename response did not contain the requested new identifier: ${out}")
endif()


if(NOT out MATCHES "hoverProvider")
  message(FATAL_ERROR "NEXUS LSP did not advertise hover: ${out}")
endif()
string(FIND "${out}" "\"id\":8,\"result\":{\"contents\":{\"kind\":\"markdown\"}" hover_response_pos)
string(FIND "${out}" "**print**" hover_doc_pos)
if(hover_response_pos EQUAL -1 OR hover_doc_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP did not return the built-in print hover documentation: ${out}")
endif()

file(REMOVE "${LSP_INPUT}")
message(STATUS "NEXUS LSP live diagnostics smoke test: PASS")
