# Linter

`nexus lint` runs the current line-level checks and then parses and semantically analyzes one source file. With no file argument, it resolves the current project's configured entry source.

## Usage

```sh
# Lint the project's entry source
nexus lint

# Check a specific source file
nexus lint src/main.nx
```

## Current checks

| Code | Trigger | Behavior |
|---|---|---|
| `NX9001` | Trailing spaces or tabs at the end of a source line | Prints a warning with the file and line |
| `NX9002` | A line contains `TODO` or `FIXME` | Prints a warning with the file and line |

After these checks, NEXUS parses the program and runs its semantic analyzer. Syntax or semantic errors fail the command. Warnings are printed but do not currently fail the command by themselves.

## Limitations

The linter is an early implementation. It does not yet have configurable rules, automatic fixes, project-wide recursive file discovery, JSON/SARIF output, unused-variable analysis, dead-code analysis, or warning suppression attributes. The `--rule`, `--fix`, and `--format` options shown in older examples are not implemented.

Use `nexus fmt --check src/main.nx` to verify the currently supported formatter behavior without modifying the source file. For the roadmap and planned static-analysis work, see [NEXUS Roadmap](../development/roadmap.md).
