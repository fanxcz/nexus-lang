# Formatter

`nexus fmt` formats NEXUS source with conservative whitespace and indentation rules.

## Usage

```sh
# Format the configured entry source
nexus fmt

# Format a single file
nexus fmt src/main.nx

# Check without writing changes
nexus fmt --check src/main.nx

# Format all project .nx files (excluding dependency/build directories)
nexus fmt --all

# Check all project .nx files; return nonzero if any need formatting
nexus fmt --check --all
```

Check mode leaves the files untouched. It exits successfully when the selected files are already formatted and returns a nonzero status when formatting changes would be needed or a file cannot be read.

## What it changes — and what it protects

The formatter respects quoted strings, line comments, and multiline block comments. It normalizes spaces around operators and common punctuation, removes trailing whitespace, and indents blocks by four spaces based on braces in code rather than braces in strings or comments. Original line breaks and comments are kept; the formatter does not rebuild code from the AST or wrap long expressions.

For source that already parses, the formatter compares the lexer token stream before and after formatting. If token kinds or token text differ, or the output no longer parses, it returns the original source unchanged. This is deliberately stricter than only checking whether the result parses: two different token streams can both be valid but mean different things. Incomplete source can still be formatted conservatively, so editor formatting remains useful while a file is being written.

This is a conservative source formatter, not a full AST pretty-printer. It does not split multiple statements written on one line or reflow long expressions. Review formatting changes for intentionally unusual layouts.

`nexus fmt --all` walks project `.nx` files in deterministic path order and skips `vendor`, `build`, `dist`, `.git`, `.vscode`, `node_modules`, `target`, `.cache`, and `CMakeFiles`. The CLI formats whole files; the language server also supports selected-range formatting in editors.

For a local hook, run the check on the source files you care about:

```sh
#!/bin/sh
nexus fmt --check src/main.nx
```

See the [roadmap](../development/roadmap.md) for broader language and tooling priorities.
