if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()

set(LSP_INPUT "${CMAKE_BINARY_DIR}/lsp-smoke.input")
set(LSP_WORKSPACE "${CMAKE_BINARY_DIR}/lsp-workspace")
file(REMOVE_RECURSE "${LSP_WORKSPACE}")
file(MAKE_DIRECTORY "${LSP_WORKSPACE}")
file(WRITE "${LSP_WORKSPACE}/nexus.toml" "[package]\nname = \"lsp-smoke\"\nversion = \"0.1.0\"\n")
file(WRITE "${LSP_WORKSPACE}/disk_helper.nx" "fn disk_helper() {}\n")
file(TO_CMAKE_PATH "${LSP_WORKSPACE}" LSP_WORKSPACE_URI_PATH)
if(WIN32)
  set(LSP_WORKSPACE_URI "file:///${LSP_WORKSPACE_URI_PATH}")
else()
  set(LSP_WORKSPACE_URI "file://${LSP_WORKSPACE_URI_PATH}")
endif()
set(workspace_caller_uri "${LSP_WORKSPACE_URI}/caller.nx")
set(workspace_helper_uri "${LSP_WORKSPACE_URI}/disk_helper.nx")
set(init "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"capabilities\":{\"workspace\":{\"didChangeWatchedFiles\":{\"dynamicRegistration\":true}}}}}")
set(initialized "{\"jsonrpc\":\"2.0\",\"method\":\"initialized\",\"params\":{}}")
set(watcher_registration_response "{\"jsonrpc\":\"2.0\",\"id\":\"nexus-register-nx-watcher\",\"result\":null}")
set(open_bad "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn main() {\\n  print(1\\n}\\n\"}}}")
set(change_good "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\",\"version\":2},\"contentChanges\":[{\"text\":\"fn main() {\\n  print(42)\\n}\\n\"}]}}")
set(change_semantic "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\",\"version\":3},\"contentChanges\":[{\"text\":\"fn main() { print(unknown) }\"}]}}")
set(close_msg "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didClose\",\"params\":{\"textDocument\":{\"uri\":\"file:///broken.nx\"}}}")
set(completion "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///hello.nx\"},\"position\":{\"line\":0,\"character\":0}}}")
set(open_definition "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn helper() {}\\nfn main() { helper() }\"}}}")
set(open_caller "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///caller.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn caller() { helper() }\"}}}")
set(open_hover "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn main() { print(42) }\"}}}")
set(open_user_hover "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover_user.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"struct Person { name: string, age: i64 }\\nenum Color { Red, Green }\\nfn add(a: i64, b: i64) -> i64 { return a + b }\\nfn main() { let result: i64 = add(1, 2) }\"}}}")
set(definition "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14}}}")
set(formatting "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/formatting\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"options\":{\"tabSize\":4,\"insertSpaces\":true}}}")
set(references "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"textDocument/references\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14},\"context\":{\"includeDeclaration\":true}}}")
set(rename "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"textDocument/rename\",\"params\":{\"textDocument\":{\"uri\":\"file:///definition.nx\"},\"position\":{\"line\":1,\"character\":14},\"newName\":\"renamed_helper\"}}")
set(hover "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover.nx\"},\"position\":{\"line\":0,\"character\":13}}}")
set(hover_user "{\"jsonrpc\":\"2.0\",\"id\":9,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover_user.nx\"},\"position\":{\"line\":2,\"character\":4}}}")
set(hover_struct "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover_user.nx\"},\"position\":{\"line\":0,\"character\":9}}}")
set(hover_enum "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///hover_user.nx\"},\"position\":{\"line\":1,\"character\":6}}}")
set(open_workspace_caller "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"${workspace_caller_uri}\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn caller() { disk_helper() }\\n\"}}}")
set(workspace_definition "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"${workspace_caller_uri}\"},\"position\":{\"line\":0,\"character\":16}}}")
set(workspace_references "{\"jsonrpc\":\"2.0\",\"id\":13,\"method\":\"textDocument/references\",\"params\":{\"textDocument\":{\"uri\":\"${workspace_caller_uri}\"},\"position\":{\"line\":0,\"character\":16},\"context\":{\"includeDeclaration\":true}}}")
set(workspace_rename "{\"jsonrpc\":\"2.0\",\"id\":14,\"method\":\"textDocument/rename\",\"params\":{\"textDocument\":{\"uri\":\"${workspace_caller_uri}\"},\"position\":{\"line\":0,\"character\":16},\"newName\":\"renamed_disk_helper\"}}}")
set(open_scope "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///scope.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn main() {\\n    let value = 1\\n    if true {\\n        let value = 2\\n        print(value)\\n    }\\n    print(value)\\n}\\nfn other() { let value = 3; print(value) }\\n\"}}}")
set(scope_references "{\"jsonrpc\":\"2.0\",\"id\":15,\"method\":\"textDocument/references\",\"params\":{\"textDocument\":{\"uri\":\"file:///scope.nx\"},\"position\":{\"line\":4,\"character\":14},\"context\":{\"includeDeclaration\":true}}}")
set(scope_rename "{\"jsonrpc\":\"2.0\",\"id\":16,\"method\":\"textDocument/rename\",\"params\":{\"textDocument\":{\"uri\":\"file:///scope.nx\"},\"position\":{\"line\":4,\"character\":14},\"newName\":\"innerValue\"}}")
set(scope_definition "{\"jsonrpc\":\"2.0\",\"id\":17,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"file:///scope.nx\"},\"position\":{\"line\":4,\"character\":14}}}")
set(open_completion_fields "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///completion_fields.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"struct Person { name: string, age: i64 }\\nfn main() { let person: Person = Person{name: \\\"A\\\", age: 1}; person. }\"}}}")
set(field_completion "{\"jsonrpc\":\"2.0\",\"id\":18,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///completion_fields.nx\"},\"position\":{\"line\":1,\"character\":67}}}")
set(open_completion_enum "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///completion_enum.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"enum Color { Red, Green }\\nfn main() { let color = Color::Red; Color:: }\"}}}")
set(enum_completion "{\"jsonrpc\":\"2.0\",\"id\":19,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///completion_enum.nx\"},\"position\":{\"line\":1,\"character\":43}}}")
set(open_multi_syntax "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///multi_syntax.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"fn first( { }\\nfn second( { }\\nfn main() { }\"}}}")
set(open_unknown_field "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///unknown_field.nx\",\"languageId\":\"nx\",\"version\":1,\"text\":\"struct Person { name: string }\\nfn main() { let person: Person = Person{name: \\\"A\\\"}; print(person.age) }\"}}}")
set(shutdown "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"shutdown\",\"params\":null}")
set(exit_msg "{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":null}")

set(input_data "")
foreach(message IN ITEMS "${init}" "${initialized}" "${watcher_registration_response}" "${open_bad}" "${change_good}" "${change_semantic}" "${close_msg}" "${completion}" "${open_definition}" "${open_caller}" "${open_hover}" "${open_user_hover}" "${definition}" "${formatting}" "${references}" "${rename}" "${hover}" "${hover_user}" "${hover_struct}" "${hover_enum}" "${open_workspace_caller}" "${workspace_definition}" "${workspace_references}" "${workspace_rename}" "${open_scope}" "${scope_references}" "${scope_rename}" "${scope_definition}" "${open_completion_fields}" "${field_completion}" "${open_completion_enum}" "${enum_completion}" "${open_multi_syntax}" "${open_unknown_field}" "${shutdown}" "${exit_msg}")
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
   OR NOT out MATCHES "client/registerCapability"
   OR NOT out MATCHES "workspace/didChangeWatchedFiles"
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

if(NOT out MATCHES "\"label\":\"name\"" OR
   NOT out MATCHES "\"label\":\"age\"" OR
   NOT out MATCHES "\"label\":\"Color::Red\"" OR
   NOT out MATCHES "\"label\":\"Color::Green\"")
  message(FATAL_ERROR "NEXUS LSP did not return struct-field and enum-variant completions: ${out}")
endif()


string(FIND "${out}" "\"globPattern\":\"**/*.nx\"" watcher_glob_position)
if(watcher_glob_position EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP watcher registration did not include the expected .nx glob: ${out}")
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
string(FIND "${out}" "**print**" hover_doc_pos)
if(hover_doc_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP did not return the built-in print hover documentation: ${out}")
endif()


string(FIND "${out}" "\"id\":9,\"result\":{\"contents\":{\"kind\":\"markdown\"" user_hover_response_pos)
string(FIND "${out}" "fn add(a: i64, b: i64) -> i64" user_hover_signature_pos)
if(user_hover_response_pos EQUAL -1 OR user_hover_signature_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP did not return the user-defined function signature: ${out}")
endif()


string(FIND "${out}" "**struct Person**" user_struct_hover_pos)
string(FIND "${out}" "Fields:" user_struct_fields_pos)
string(FIND "${out}" "**enum Color**" user_enum_hover_pos)
string(FIND "${out}" "Variants:" user_enum_variants_pos)
if(user_struct_hover_pos EQUAL -1 OR user_struct_fields_pos EQUAL -1 OR
   user_enum_hover_pos EQUAL -1 OR user_enum_variants_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP did not return user-defined struct and enum hover details: ${out}")
endif()


string(FIND "${out}" "\"id\":12,\"result\":{\"uri\":\"${workspace_helper_uri}\"" workspace_definition_pos)
if(workspace_definition_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP did not resolve a definition in an unopened project file: ${out}")
endif()
string(FIND "${out}" "\"id\":13,\"result\":[" workspace_references_pos)
string(FIND "${out}" "\"id\":14,\"result\":{\"changes\":" workspace_rename_pos)
if(workspace_references_pos EQUAL -1 OR workspace_rename_pos EQUAL -1 OR workspace_rename_pos LESS workspace_references_pos)
  message(FATAL_ERROR "NEXUS LSP did not return workspace references and rename edits: ${out}")
endif()
math(EXPR workspace_references_length "${workspace_rename_pos} - ${workspace_references_pos}")
string(SUBSTRING "${out}" ${workspace_references_pos} ${workspace_references_length} workspace_references_payload)
string(FIND "${workspace_references_payload}" "\"uri\":\"${workspace_helper_uri}\"" workspace_disk_reference_pos)
string(FIND "${workspace_references_payload}" "\"uri\":\"${workspace_caller_uri}\"" workspace_caller_reference_pos)
if(workspace_disk_reference_pos EQUAL -1 OR workspace_caller_reference_pos EQUAL -1)
  message(FATAL_ERROR "Expected references in the unopened declaration and open caller file: ${workspace_references_payload}")
endif()
string(SUBSTRING "${out}" ${workspace_rename_pos} -1 workspace_rename_payload)
string(FIND "${workspace_rename_payload}" "disk_helper.nx" workspace_disk_rename_pos)
string(FIND "${workspace_rename_payload}" "\"newText\":\"renamed_disk_helper\"" workspace_rename_text_pos)
if(workspace_disk_rename_pos EQUAL -1 OR workspace_rename_text_pos EQUAL -1)
  message(FATAL_ERROR "Workspace rename did not include unopened file edits: ${workspace_rename_payload}")
endif()

string(FIND "${out}" "\"id\":15,\"result\":[" scope_references_pos)
string(FIND "${out}" "\"id\":16,\"result\":{\"changes\":" scope_rename_pos)
if(scope_references_pos EQUAL -1 OR scope_rename_pos EQUAL -1 OR scope_rename_pos LESS scope_references_pos)
  message(FATAL_ERROR "NEXUS LSP did not return scoped reference and rename responses: ${out}")
endif()
math(EXPR scope_references_length "${scope_rename_pos} - ${scope_references_pos}")
string(SUBSTRING "${out}" ${scope_references_pos} ${scope_references_length} scope_references_payload)
string(REGEX MATCHALL "\"uri\":\"file:///scope.nx\"" scoped_reference_locations "${scope_references_payload}")
list(LENGTH scoped_reference_locations scoped_reference_count)
if(NOT scoped_reference_count EQUAL 2)
  message(FATAL_ERROR "Expected only the inner variable declaration and its use; unrelated shadowed variables must not appear: ${scope_references_payload}")
endif()
string(SUBSTRING "${out}" ${scope_rename_pos} -1 scope_rename_payload)
string(FIND "${scope_rename_payload}" "\"newText\":\"innerValue\"" scoped_rename_text_pos)
string(FIND "${scope_rename_payload}" "\"line\":1,\"character\":8" outer_declaration_pos)
string(FIND "${scope_rename_payload}" "\"line\":6,\"character\":14" outer_reference_pos)
string(FIND "${scope_rename_payload}" "\"line\":8" other_function_pos)
if(scoped_rename_text_pos EQUAL -1 OR NOT outer_declaration_pos EQUAL -1 OR
   NOT outer_reference_pos EQUAL -1 OR NOT other_function_pos EQUAL -1)
  message(FATAL_ERROR "Scoped rename changed symbols outside the selected local binding: ${scope_rename_payload}")
endif()

string(FIND "${out}" "\"id\":17,\"result\":{\"uri\":\"file:///scope.nx\",\"range\":{\"start\":{\"line\":3,\"character\":12}" scope_definition_pos)
if(scope_definition_pos EQUAL -1)
  message(FATAL_ERROR "NEXUS LSP definition should resolve to the shadowing local variable, not the outer variable: ${out}")
endif()


string(FIND "${out}" "\"uri\":\"file:///multi_syntax.nx\"" multi_syntax_pos)
string(SUBSTRING "${out}" ${multi_syntax_pos} -1 multi_syntax_payload)
string(FIND "${multi_syntax_payload}" "\"line\":0,\"character\":10" first_syntax_error_pos)
string(FIND "${multi_syntax_payload}" "\"line\":1,\"character\":11" second_syntax_error_pos)
if(multi_syntax_pos EQUAL -1 OR first_syntax_error_pos EQUAL -1 OR second_syntax_error_pos EQUAL -1)
  message(FATAL_ERROR "LSP multi-error recovery diagnostic range regression: ${multi_syntax_payload}")
endif()


string(FIND "${out}" "\"uri\":\"file:///unknown_field.nx\"" unknown_field_pos)
string(SUBSTRING "${out}" ${unknown_field_pos} -1 unknown_field_payload)
string(FIND "${unknown_field_payload}" "\"line\":1,\"character\":65" unknown_field_start_pos)
string(FIND "${unknown_field_payload}" "\"line\":1,\"character\":68" unknown_field_end_pos)
if(unknown_field_pos EQUAL -1 OR unknown_field_start_pos EQUAL -1 OR unknown_field_end_pos EQUAL -1)
  message(FATAL_ERROR "LSP semantic field diagnostic should highlight the missing field name: ${unknown_field_payload}")
endif()

file(REMOVE "${LSP_INPUT}")
message(STATUS "NEXUS LSP live diagnostics smoke test: PASS")
