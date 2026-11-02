# NEXUS Source Style Guide

This guide defines a consistent style for examples and `stdlib/`. It is a project convention, not a claim that every rule is enforced by the parser.

## Formatting

- Use four spaces for indentation; do not use tabs.
- Put one statement per line in examples and standard-library code.
- Put spaces around binary operators and after commas.
- Put the opening brace on the same line as the declaration/control statement; put the closing brace on its own line.
- Keep functions focused and reasonably small.
- Run `nexus fmt` on changed NEXUS sources. Before submitting, use `nexus fmt --check --all`.

## Naming

- Use `snake_case` for functions, variables, parameters and module file names.
- Use descriptive nouns for values and verbs for functions (for example, `read_config`, `parse_token`).
- Use `PascalCase` for user-defined type names where a type needs a distinct visual identity.
- Use stable descriptive names for enum variants; avoid abbreviations unless conventional in the domain.
- Keep public standard-library names short, consistent and documented; avoid duplicate aliases for the same operation.

## Types and error handling

- Add explicit types to public function parameters and return values when the contract benefits from it.
- Prefer the narrowest numeric type that matches the API contract; document signedness and units.
- Do not rely on implicit integer narrowing. Make conversions explicit and validate external values.
- Use the concrete `Option` and `Result` helpers where they make absence or failure part of the API contract.
- Check I/O, filesystem and runtime return values. Do not silently discard failures in examples.

## Imports and modules

- Put imports before declarations.
- Use `import std.NAME` for standard modules and quoted relative paths for local source files.
- Keep the import graph acyclic; the compiler reports circular imports.
- Do not document `module`, `export`, aliases or wildcard imports as supported syntax; the current module loader composes imported source files.

## Comments and examples

- Explain why a non-obvious operation is needed; avoid comments that merely repeat the code.
- Document public standard-library functions with input constraints, return behavior and failure modes.
- Use small runnable examples where possible. Label pseudocode or incomplete fragments explicitly.
- Avoid claims that a runtime API works on every backend or host unless the support matrix and tests demonstrate it.

## Review checklist

```sh
nexus fmt --check --all
nexus lint
ctest --test-dir build --output-on-failure
```

See the language reference and the examples in `tests/native_programs/` for established syntax. If formatter output conflicts with an informal rule above, prefer the formatter and update this guide in the same change.
