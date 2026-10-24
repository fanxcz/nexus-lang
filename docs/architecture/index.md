# Architecture

Technical deep-dives into NEXUS internals.

## In This Section

| Topic | Description |
|-------|-------------|
| [Compiler Pipeline](compiler-pipeline.md) | Compilation stages |
| [Runtime Design](runtime-design.md) | Runtime architecture |
| [Memory Model](memory-model.md) | Memory management |

## Compiler Pipeline

```
Source → Lexer → Parser → Semantic → Codegen → LLVM → Binary
```

See [Compiler Pipeline](compiler-pipeline.md) for details.