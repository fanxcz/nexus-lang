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
set(definition "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14}}}")
set(formatting "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/formatting\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"options\":{\"tabSize\":4,\"insertSpaces\":true}}}")
set(shutdown "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"shutdown\",\"params\":null}")
set(exit_msg "{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":null}")

set(input_data "")
foreach(message IN ITEMS "${init}" "${open_bad}" "${change_good}" "${change_semantic}" "${close_msg}" "${completion}" "${open_definition}" "${definition}" "${formatting}" "${shutdown}" "${exit_msg}")
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

file(REMOVE "${LSP_INPUT}")
message(STATUS "NEXUS LSP live diagnostics smoke test: PASS")
