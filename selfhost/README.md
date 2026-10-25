# NEXUS self-hosting

This directory grows the compiler toolchain written in NEXUS itself. The C++
compiler in `src/` remains the reference implementation; components here are
mirrored against it feature by feature and must not diverge.

## Current components

| Component | File | Status |
|---|---|---|
| Lexer | `lexer_lib.nx` | Works; covers the full token set of `src/compiler/frontend/lexer/lexer.cpp` |
| Lexer tests | `lexer_test.nx` | 10 cases: keywords, numbers, operators, escapes, comments, 4 error cases |
| Self-lexing smoke | `lex_self.nx` | The NEXUS lexer tokenizes its own sources plus real stdlib/example files |
| Parser (subset) | `parser_lib.nx` | Works; recursive descent with correct precedence for expressions plus `let`/assignment/`print`/`if`/`else`/`while`/blocks/`fn`/`return` |
| Parser tests | `parser_test.nx` | 12 cases: precedence, associativity, calls, blocks, 3 error cases |
| Type checker (subset) | `typecheck_lib.nx` | Works; variable environment, numeric/string/bool rules, bool conditions for `if`/`while`, assignment type stability |
| Type checker tests | `typecheck_test.nx` | 13 cases: 3 valid programs, 10 diagnostics |
| Code generator (subset) | `codegen_lib.nx` | Works; emits a complete LLVM IR module (alloca/load/store SSA, branches, loops) for the i64 statement subset, plus function declarations/calls and string literals in print |
| Code generator tests | `codegen_test.nx` | End-to-end: NEXUS compiles `tests/hello_subset.nx` and `tests/fn_subset.nx` to `.ll`, clang++ builds them, the binaries run and print the expected outputs |

`tokenize(src: string) -> string` emits one token per line as
`LINE:COL KIND TEXT` (`Integer 42`, `KwFn fn`, `String decoded-text`, ...).
On the first lexical error it stops and emits `error LINE:COL MESSAGE`,
matching the reference lexer's failure points: unterminated strings, unknown
escapes, unexpected characters, unterminated block comments.

The lexer self-lexes cleanly: `nexus run selfhost/lex_self.nx --backend llvm`
tokenizes `lexer_lib.nx` (~1800 tokens), `lex_self.nx` itself and several
stdlib/example sources with zero errors. `lexer_test.nx` is excluded from
self-lexing on purpose because its source embeds invalid inputs for the
negative test cases.

## Tests

`ctest -R selfhost` runs both programs through the LLVM backend and fails on
any non-zero exit or unexpected output. The test needs a clang-compatible
`NEXUS_CXX`; plain g++ cannot link the `.ll` pipeline.

## Roadmap

1. ~~Lexer~~ (done)
2. ~~Parser for the statement/expression subset~~ (done; `fn` declarations, structs, enums, `match`, `for` and typed annotations remain)
2b. Type checking for the parsed subset, verified against the C++ parser's
   acceptance/rejection on the same inputs
3. Symbol table and type checking for the subset the lexer covers
4. Code generation through the existing LLVM IR text pipeline — the NEXUS
   compiler would then emit `.ll` for simple programs
5. Only after all of the above: attempt to compile `lexer_lib.nx` with the
   NEXUS-written compiler. Self-hosting is declared only when that binary
   reproduces the token stream of the C++ pipeline on the whole test corpus.

## Constraints found while writing NEXUS-in-NEXUS

- No dynamically sized arrays: token streams are accumulated as strings
  instead. A `Vec`-style builtin or growable array type is the single most
  useful language extension for the parser stage.
- No string character type: all scanning is done with `string.byte_at` and
  byte comparisons, which works but makes the code verbose.
- Structs cannot be passed by reference into helper functions, so the lexer
  is one function rather than a state object plus helpers.
