# Language Overview

NEXUS is a statically-typed, compiled systems programming language with a syntax inspired by Rust, C, and modern languages. It targets LLVM for native code generation.

## Design Goals

- **Performance**: Zero-cost abstractions, no GC, manual memory management
- **Safety**: Strong typing, explicit error handling, no null by default
- **Expressiveness**: Modern syntax, pattern matching, modules, generics (planned)
- **Interoperability**: Seamless C/FFI, direct memory access
- **Game Development**: Built-in runtime for graphics, audio, ECS, physics

## Hello World

```nx
import std.io

fn main() {
    print("Hello, NEXUS!")
}
```

## Basic Syntax

### Variables

```nx
// Immutable by default
let x = 42
let name = "NEXUS"
let pi = 3.14159

// Mutable with mut
mut counter = 0
counter += 1

// Type annotations (optional, inferred)
let x: i32 = 42
let name: str = "NEXUS"
let items: []i32 = [1, 2, 3]
```

### Primitive Types

| Type | Description | Size |
|------|-------------|------|
| `bool` | Boolean | 1 byte |
| `i8` / `u8` | Signed/unsigned 8-bit | 1 byte |
| `i16` / `u16` | Signed/unsigned 16-bit | 2 bytes |
| `i32` / `u32` | Signed/unsigned 32-bit | 4 bytes |
| `i64` / `u64` | Signed/unsigned 64-bit | 8 bytes |
| `isize` / `usize` | Pointer-sized integer | Platform |
| `f32` | 32-bit float | 4 bytes |
| `f64` | 64-bit float | 8 bytes |
| `char` | Unicode scalar value | 4 bytes |
| `str` | UTF-8 string slice | Fat pointer |
| `*T` | Raw pointer to T | Platform |
| `&T` / `&mut T` | Reference (borrowed) | Platform |

### Functions

```nx
fn add(a: i32, b: i32) -> i32 {
    a + b  // Implicit return (last expression)
}

fn greet(name: str) {
    print("Hello, {name}!")
}

// Multiple return values
fn divmod(a: i32, b: i32) -> (i32, i32) {
    (a / b, a % b)
}

let (q, r) = divmod(10, 3)
```

### Control Flow

```nx
// If expressions
let max = if a > b { a } else { b }

// Match expressions (pattern matching)
let result = match value {
    0 => "zero",
    1..=10 => "small",
    n if n > 100 => "large",
    _ => "medium",
}

// Loops
while condition {
    // ...
}

for i in 0..10 {
    // Range loop
}

for item in items {
    // Iterator loop
}

loop {
    // Infinite loop
    if done { break }
}
```

### Structs and Enums

```nx
// Struct
struct Vec2 {
    x: f32,
    y: f32,
}

impl Vec2 {
    fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
    
    fn length(&self) -> f32 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
}

// Enum (tagged union)
enum Option<T> {
    Some(T),
    None,
}

enum Result<T, E> {
    Ok(T),
    Err(E),
}

// Usage
let opt: Option<i32> = Some(42)
match opt {
    Some(v) => print("Value: {v}"),
    None => print("No value"),
}
```

### Modules

```nx
// math.nx
module math {
    export const PI: f64 = 3.14159265359
    
    export fn sin(x: f64) -> f64 { ... }
    export fn cos(x: f64) -> f64 { ... }
}

// main.nx
import math
import math as m  // alias

fn main() {
    print(math.PI)
    print(m.sin(0.0))
}
```

### Error Handling

```nx
// Result-based error handling
fn read_file(path: str) -> Result<str, IoError> {
    // ...
}

// Using match
match read_file("data.txt") {
    Ok(content) => print(content),
    Err(e) => print("Error: {e}"),
}

// Using try operator (?)
fn process() -> Result<(), IoError> {
    let content = read_file("data.txt")?
    let parsed = parse(content)?
    print(parsed)
    Ok(())
}
```

### Memory Management

```nx
// Manual allocation
let ptr = mem_alloc(1024)  // *u8
defer mem_free(ptr)        // RAII-style cleanup

// Struct allocation
let vec = mem_alloc(sizeof(Vec2))
let v = cast<&mut Vec2>(vec)
v.x = 1.0
v.y = 2.0

// Smart pointers (planned)
// let box = Box::new(Vec2::new(1, 2))
// let rc = Rc::new(Vec2::new(1, 2))
```

### FFI (Foreign Function Interface)

```nx
// Link C library
extern "C" {
    fn printf(format: *const i8, ...) -> i32
    fn malloc(size: usize) -> *mut u8
    fn free(ptr: *mut u8)
}

// Call C function
fn main() {
    unsafe {
        printf(c"Hello from C!\n")
    }
}
```

## Current Limitations (v1.1.0)

- Generics: Basic support, full monomorphization in progress
- Traits/Interfaces: Not yet implemented
- Closures: Syntax exists, lowering to LLVM in progress
- Async/Await: Not yet implemented
- Pattern matching: Basic match on integers/enums only

See [CHANGELOG](https://github.com/fanxcz/nexus/blob/main/CHANGELOG.md) for version history.