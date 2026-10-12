# Formatter

`nexus fmt` applies the current NEXUS source formatting rules to one file. With no file argument, it resolves the current project's configured entry source.

## Usage

```bash
# Format the project's configured entry source
nexus fmt

# Format one source file
nexus fmt src/main.nx

# Check formatting without modifying the file
nexus fmt --check src/main.nx
```

The check command exits with status 0 when the file is already formatted, and a nonzero status when the formatter would change it. It intentionally does not write in check mode, making it suitable for CI and pre-commit hooks.

## Current behavior and limits

The formatter currently trims trailing whitespace and applies a line-based indentation heuristic around braces. It is **not yet a syntax-aware pretty-printer**: it does not normalize spaces around every token, wrap long expressions, or recursively discover all source files. Review formatting changes for complex single-line blocks and comments.

For a local hook, use a small script that checks the entry file:

```sh
#!/bin/sh
nexus fmt --check src/main.nx
```

The formatter will be expanded after parser-aware formatting rules and regression fixtures are in place. See the [roadmap](../development/roadmap.md).
