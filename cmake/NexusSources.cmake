# Source groups for the NEXUS compiler and its CTest executable.
# Paths are relative to the repository root, where add_executable() is invoked.

# ============================================================================
set(NEXUS_DRIVER_SOURCES
    src/compiler/driver/driver.cpp
    src/compiler/driver/cli/usage.cpp
    src/compiler/driver/commands/doctor.cpp
    src/compiler/driver/commands/export.cpp
    src/compiler/driver/commands/format.cpp
    src/compiler/driver/commands/info.cpp
    src/compiler/driver/commands/quality.cpp
    src/compiler/driver/lsp/server.cpp
    src/compiler/driver/lsp/protocol.cpp
    src/compiler/driver/lsp/text.cpp
    src/compiler/driver/lsp/uri.cpp
    src/compiler/driver/lsp/diagnostics.cpp
    src/compiler/driver/package/command.cpp
    src/compiler/driver/package/lockfile.cpp
    src/compiler/driver/package/metadata.cpp
    src/compiler/driver/package/vendor.cpp
    src/compiler/driver/project/config.cpp
    src/compiler/driver/project/modules.cpp
    src/compiler/driver/project/templates.cpp
    src/compiler/driver/repl/command.cpp
    src/compiler/driver/repl/input.cpp
    src/compiler/driver/repl/repl_classify.cpp
    src/compiler/driver/repl/repl_history.cpp
    src/compiler/driver/support/diagnostics.cpp
    src/compiler/driver/support/manifest.cpp
    src/compiler/driver/support/process.cpp
    src/compiler/driver/support/text.cpp
    src/compiler/driver/support/toml.cpp
    src/compiler/driver/toolchain/compile.cpp
    src/compiler/driver/toolchain/exec.cpp
    src/compiler/driver/toolchain/paths.cpp
    src/compiler/driver/toolchain/runtime.cpp
)

set(NEXUS_CODEGEN_SOURCES
    src/compiler/backend/codegen/llvm_ir.cpp
    src/compiler/backend/codegen/llvm_cg_core.cpp
    src/compiler/backend/codegen/llvm_cg_expr.cpp
    src/compiler/backend/codegen/llvm_cg_stmt.cpp
    src/compiler/backend/codegen/llvm_cg_program.cpp
)

set(NEXUS_NATIVE_SOURCES
    src/compiler/backend/native/native.cpp
    src/compiler/backend/native/native_subset.cpp
    src/compiler/backend/native/native_gen_core.cpp
    src/compiler/backend/native/native_gen_stmt.cpp
    src/compiler/backend/native/native_gen_runtime.cpp
    src/compiler/backend/native/target.cpp
    src/compiler/backend/native/pe_elf.cpp
)

set(NEXUS_LEXER_SOURCES
    src/compiler/frontend/lexer/lexer.cpp
    src/compiler/frontend/lexer/lexer_token.cpp
    src/compiler/frontend/lexer/lexer_literal.cpp
    src/compiler/frontend/lexer/lexer_scan.cpp
)

set(NEXUS_PARSER_SOURCES
    src/compiler/frontend/parser/parser.cpp
    src/compiler/frontend/parser/parser_support.cpp
    src/compiler/frontend/parser/parser_stmt.cpp
    src/compiler/frontend/parser/parser_expr.cpp
)

set(NEXUS_SEMANTIC_SOURCES
    src/compiler/semantic/semantic.cpp
    src/compiler/semantic/semantic_expr.cpp
    src/compiler/semantic/semantic_stmt.cpp
)

set(NEXUS_SOURCES
    src/cli/main.cpp
    src/compiler/format/formatter.cpp
    ${NEXUS_LEXER_SOURCES}
    ${NEXUS_PARSER_SOURCES}
    ${NEXUS_SEMANTIC_SOURCES}
    ${NEXUS_CODEGEN_SOURCES}
    ${NEXUS_NATIVE_SOURCES}
    ${NEXUS_DRIVER_SOURCES}
)
