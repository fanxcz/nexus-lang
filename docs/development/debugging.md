# Debugging

Debugging NEXUS compiler and applications.

## Debugging the Compiler

### Build Debug Version

```bash
cmake --preset=debug
cmake --build --preset=debug
```

### GDB / LLDB

```bash
# GDB
gdb ./build/nexus
(gdb) run new test --template app
(gdb) bt

# LLDB
lldb ./build/nexus
(lldb) run new test --template app
(lldb) bt
```

### Common Breakpoints

```gdb
# Lexer
break lexer.cpp:42

# Parser
break parser.cpp:123

# Semantic
break semantic.cpp:567

# Codegen
break llvm_ir.cpp:89

# Panic handler
break panic_handler
```

### Debugging Tips

```bash
# Enable debug logging
./build/nexus --debug build src/main.nx

# Dump AST
./build/nexus --dump-ast src/main.nx

# Dump LLVM IR
./build/nexus --emit-llvm src/main.nx

# Verbose compilation
./build/nexus -v build src/main.nx
```

## Debugging Applications

### Print Debugging

```nx
fn main() {
    let x = compute()
    print("DEBUG: x = {x}")  // Simple
    eprintln("DEBUG: x = {x}")  // To stderr
    
    // With location
    print("DEBUG [{}:{}]: x = {x}", file!(), line!())
}
```

### Assertions

```nx
fn divide(a: f64, b: f64) -> f64 {
    assert(b != 0.0, "Division by zero")
    a / b
}

// Debug-only (removed in release)
debug_assert!(ptr.is_not_null())
```

### GDB with NEXUS Programs

```bash
# Compile with debug info (default)
nexus build

# Debug
gdb ./build/myapp
(gdb) run
(gdb) break main.nx:42
(gdb) print variable_name
```

### Memory Debugging

```bash
# AddressSanitizer
cmake --preset=sanitize-address
cmake --build --preset=sanitize-address

# Valgrind (Linux)
valgrind --leak-check=full ./build/myapp

# Heap tracking
# In code:
mem_track_enable(true)
// ... run ...
mem_track_report()
mem_track_disable()
```

### Common Issues

| Symptom | Check |
|---------|-------|
| Segfault | Null pointer, use-after-free, stack overflow |
| Wrong output | Logic error, operator precedence |
| Infinite loop | Loop condition, iterator invalidation |
| Slow | Algorithm complexity, allocations in loop |
| Crash on exit | Double free, destructor order |

### VS Code Debug Config

```json
// .vscode/launch.json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug NEXUS Compiler",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/nexus",
            "args": ["build", "src/main.nx"],
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb"
        },
        {
            "name": "Debug NEXUS App",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/myapp/build/myapp",
            "cwd": "${workspaceFolder}/myapp",
            "MIMode": "gdb"
        }
    ]
}
```

### CLion Debug Config

1. Run → Edit Configurations → +
2. **CMake Application**
3. Target: `nexus` or your app
4. Executable: `build/nexus`
5. Arguments: `build src/main.nx`

## Profiling

```bash
# perf (Linux)
perf record ./build/nexus build src/main.nx
perf report

# Instruments (macOS)
xcrun xctrace record --template "Time Profiler" --launch -- ./build/nexus

# VTune (Intel)
vtune -collect hotspots -r result -- ./build/nexus
```

## Logging

```bash
# Compiler debug logs
./build/nexus --log-level debug build src/main.nx

# Runtime debug
export NEXUS_DEBUG=1
./build/myapp
```