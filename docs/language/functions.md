# Functions

The current compiler supports named functions with typed parameters and an optional return type. A function without an explicit return type returns `void`; non-void functions must use an explicit `return` on every control-flow path.

## Define and call a function

```nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}

fn main() {
    print(add(40, 2))
}
```

Functions can call other functions and can be recursive. Function names are unique in a program; declaring a second function with the same name is an error. The current compiler does not provide overloads.

## Parameters and return values

Parameters use `name: type` syntax. Supported scalar types include `i8`/`i16`/`i32`/`i64`, `u8`/`u16`/`u32`/`u64`, `f32`/`f64`, `bool` and `string`; types can also be user-defined structs/enums, fixed arrays and pointers.

```nx
fn area(width: f64, height: f64) -> f64 {
    return width * height
}

fn describe(enabled: bool) -> string {
    if enabled {
        return "enabled"
    } else {
        return "disabled"
    }
}
```

Use an explicit return type and `return` rather than relying on Rust-style implicit tail-expression returns.

## Struct data

The parser supports structs as named fields and permits field access/assignment:

```nx
struct Point {
    x: f64
    y: f64
}

fn move_x(point: Point, amount: f64) -> Point {
    return Point { x: point.x + amount, y: point.y }
}
```

The current language does not support Rust-style `impl` blocks or methods such as `Point::new`. Put helper functions at top level and pass the struct explicitly.

## Pointers

Pointer types use the `*Type` syntax, and the language supports address-of and dereference expressions in its current pointer subset:

```nx
fn read_value(value: *i64) -> i64 {
    return *value
}

fn main() {
    let count: i64 = 7
    let ptr: *i64 = &count
    print(read_value(ptr))
}
```

Pointers are low-level; only pass valid addresses of compatible values and avoid retaining addresses beyond the lifetime of their storage. Pointer support is not a borrow checker.

## External functions

The parser supports external function declarations using an `extern` string marker:

```nx
extern "C" fn external_function(value: i64) -> i64
```

An external declaration describes a symbol; linking and ABI correctness still depend on the selected backend/toolchain and the actual library. The compiler's current grammar does not provide a variadic declaration syntax or a complete type-safe FFI layer.

## Features not supported by the current parser

Generics, function-pointer/closure values, capture closures, `defer`, `impl` methods, attributes such as `#[inline]`, trait bounds and async functions are not supported language features in this compiler version. Some may appear in roadmap notes or older drafts; do not use them in code intended to pass `nexus check`.
