# Formatter

\`nexus fmt\` formats a NEXUS source file. With no file argument, it resolves the current project's configured entry source.

## Usage

\`\`\`bash
# Format the project's configured entry source
nexus fmt

# Format one source file
nexus fmt src/main.nx

# Check formatting without modifying the file
nexus fmt --check src/main.nx
\`\`\`

The check command exits with status 0 when the file is already formatted, and a nonzero status when the formatter would change it. Check mode does not write to disk and is suitable for CI or a pre-commit hook.

## Formatting rules

The formatter now tokenizes each line while respecting quoted strings, line comments and multi-line block comments. It normalizes spacing around operators and common punctuation, removes trailing whitespace, and applies four-space indentation based on braces found in code (not braces inside strings or comments). It preserves original line breaks and comments instead of reconstructing a file from an AST.

This is a conservative source formatter, not a full AST pretty-printer. It does not wrap long expressions, split multiple statements written on one line, or automatically discover every source file in a project. Review formatting changes for deliberately unusual layouts.

For a local hook, run the check for the project's source files explicitly:

\`\`\`sh
#!/bin/sh
nexus fmt --check src/main.nx
\`\`\`

See the [roadmap](../development/roadmap.md) for planned range and project-wide formatting support.
