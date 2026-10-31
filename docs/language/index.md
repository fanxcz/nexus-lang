# Language Reference

This reference documents the syntax implemented by the NEXUS compiler. Planned features and backend-specific limits are called out separately rather than shown as if they already worked.

## In This Section

| Topic | Description |
|---|---|
| [Overview](overview.md) | Language concepts and compiler backends |
| [Syntax](syntax.md) | Valid source syntax and a first program |
| [Types](types.md) | Built-in types, structs, payload enums, arrays and pointers |
| [Functions](functions.md) | Named functions, parameters and returns |
| [Control Flow](control-flow.md) | Conditions, loops and statement-form match |
| [Imports](modules.md) | Standard-library and file-based imports |
| [Error Handling](error-handling.md) | Checked operations, concrete enum payloads, ? and defer |
| [Systems language proposal](../language-spec.md) | Proposed ownership, concurrency, SIMD, ECS and 12-week roadmap |
| [Migrating from Rust](../migration-from-rust.md) | Current limitations and a practical porting guide |
| [FFI](ffi.md) | Experimental external function declarations |

## Quick reference

### Bindings

~~~nx
let value = 42
let name = "NEXUS"
let mut count: i64 = 0
count = count + 1
const limit: i64 = 100
~~~

Bindings are immutable by default; use let mut for values that are reassigned.

### Functions

~~~nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}

fn main() {
    print(add(40, 2))
}
~~~

A non-void function requires a compatible return on every path the compiler can prove.

### Control flow

~~~nx
if value > 10 {
    print(value)
} else {
    print(0)
}

for i in 0..3 {
    print(i)
}
~~~

### User-defined data

~~~nx
struct Point {
    x: f64
    y: f64
}

enum ParseResult {
    Ok(i64),
    Err(string)
}

fn main() {
    let result = ParseResult::Ok(42)
    match result {
        ParseResult::Ok(value) => { print(value) }
        ParseResult::Err(message) => { print(message) }
    }
}
~~~

## Language limits

Generic type parameters, traits, closures, methods/impl blocks, async/await,
attributes, variadic FFI, slices, and general-purpose function types are not
implemented. Enum variants can carry one scalar payload (fixed-width integer,
f32, f64, bool, string, or pointer); generic/nested payloads are unsupported. The ? operator
works for concrete matching Some(T)/None and Ok(T)/Err enums on the LLVM
backend when the enclosing function returns that same enum type. Generic
Option<T> and Result<T, E> and cross-enum error conversion are not implemented.
defer is supported for void-returning calls on the LLVM backend; the built-in
native backend reports it as unsupported. Rust-style module/export declarations
are not supported; use the documented file-based imports.

See Type System, Error Handling, and Backend Selection for details.
