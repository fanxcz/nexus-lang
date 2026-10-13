# Formatter

\`nexus fmt\` formats a NEXUS source file. With no file argument, it resolves the current project's configured entry source.

## Usage

\`\`\`bash
# Format the project's configured entry source
nexus fmt

# Format one source file
nexus fmt src/main.nx

# Check formatting without modifying the file
nexus fmt --check src/main.nx\n\n# Format every project .nx file (excluding vendor/build outputs)\nnexus fmt --all\n\n# Check all project .nx files without modifying them\nnexus fmt --check --all
\`\`\`

The check command exits with status 0 when the file is already formatted, and a nonzero status when the formatter would change it. Check mode does not write to disk and is suitable for CI or a pre-commit hook.

## Formatting rules

The formatter now tokenizes each line while respecting quoted strings, line comments and multi-line block comments. It normalizes spacing around operators and common punctuation, removes trailing whitespace, and applies four-space indentation based on braces found in code (not braces inside strings or comments). It preserves original line breaks and comments instead of reconstructing a file from an AST.

This is a conservative source formatter, not a full AST pretty-printer. It does not wrap long expressions or split multiple statements written on one line. `nexus fmt --all` walks project `.nx` files in deterministic path order and skips `vendor`, `build`, `dist`, `.git`, `.vscode`, `node_modules`, `target`, `.cache`, and `CMakeFiles`. `nexus fmt --check --all` checks every discovered file without changing it and exits nonzero if any file needs formatting or cannot be read. Review formatting changes for deliberately unusual layouts.

For a local hook, run the check for the project's source files explicitly:

\`\`\`sh
#!/bin/sh
nexus fmt --check src/main.nx
\`\`\`

See the [roadmap](../development/roadmap.md) for planned range and project-wide formatting support.
