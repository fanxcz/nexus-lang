# Language Reference

Complete reference for the NEXUS programming language.

## In This Section

| Topic | Description |
|-------|-------------|
| [Overview](overview.md) | Language overview and basics |
| [Syntax](syntax.md) | Detailed syntax reference |
| [Types](types.md) | Type system |
| [Functions](functions.md) | Functions and methods |
| [Control Flow](control-flow.md) | If, loops, match |
| [Modules & Imports](modules.md) | Module system |
| [Error Handling](error-handling.md) | Result, Option, error propagation |
| [FFI](ffi.md) | Foreign Function Interface |

## Quick Reference

### Variables

```nx
let x = 42              // Immutable
mut x = 42              // Mutable
let x: i32 = 42         // With type annotation
```

### Functions

```nx
fn add(a: i32, b: i32) -> i32 {
    a + b
}

fn main() {
    let result = add(1, 2)
}
```

### Control Flow

```nx
if condition { } else { }

while condition { }

for item in items { }

match value {
    Pattern => expr,
    _ => default,
}
```

### Types

```nx
struct Point { x: f32, y: f32 }

enum Option<T> {
    Some(T),
    None,
}
```

### Modules

```nx
// math.nx
module math {
    export fn add(a: i32, b: i32) -> i32 { a + b }
}

// main.nx
import math
math::add(1, 2)
```