# Language Reference

This reference documents the language syntax currently implemented by the NEXUS compiler. The language is under active development; planned features are separated from supported syntax rather than presented as working examples.

## In This Section

| Topic | Description |
|---|---|
| [Overview](overview.md) | Language concepts and compiler backends |
| [Syntax](syntax.md) | Valid source syntax and a first program |
| [Types](types.md) | Built-in types, structs, enums, arrays and pointers |
| [Functions](functions.md) | Named functions, parameters and returns |
| [Control Flow](control-flow.md) | Conditions, loops and statement-form match |
| [Imports](modules.md) | Standard-library and file-based imports |
| [Error Handling](error-handling.md) | Sentinel values and explicit checks |
| [FFI](ffi.md) | Experimental external function declarations |

## Quick reference

### Bindings

```nx
let value = 42
let name = "NEXUS"
let mut count: i64 = 0
count = count + 1
const limit: i64 = 100
```

Bindings are immutable by default; use `let mut` for values that are reassigned.

### Functions

```nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}

fn main() {
    print(add(40, 2))
}
```

A non-void function requires an explicit return on every path the compiler can prove.

### Control flow

```nx
if value > 10 {
    print(value)
} else {
    print(0)
}

for i in 0..3 {
    print(i)
}
```

### User-defined data

```nx
struct Point {
    x: f64
    y: f64
}

enum Color {
    Red,
    Green,
    Blue
}
```

## Language limits

The current parser does not implement generics, traits, closures, methods/impl blocks, `defer`, `Result`/`Option`, the `?` operator, async/await, Rust-style module/export declarations, attributes, variadic FFI, slices or general-purpose function types. See the linked pages for the types of examples that are supported today.
