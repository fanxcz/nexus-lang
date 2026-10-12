# Type System

NEXUS has a strong, static type system with type inference.

## Primitive Types

| Type | Size | Range / Values |
|------|------|----------------|
| `bool` | 1 byte | `true`, `false` |
| `i8` | 1 byte | -128 to 127 |
| `u8` | 1 byte | 0 to 255 |
| `i16` | 2 bytes | -32,768 to 32,767 |
| `u16` | 2 bytes | 0 to 65,535 |
| `i32` | 4 bytes | -2³¹ to 2³¹-1 |
| `u32` | 4 bytes | 0 to 2³²-1 |
| `i64` | 8 bytes | -2⁶³ to 2⁶³-1 |
| `u64` | 8 bytes | 0 to 2⁶⁴-1 |
| `isize` | pointer | Platform-dependent |
| `usize` | pointer | Platform-dependent |
| `f32` | 4 bytes | IEEE 754 single |
| `f64` | 8 bytes | IEEE 754 double |
| `char` | 4 bytes | Unicode scalar value |
| `str` | fat ptr | UTF-8 string slice |

## Type Aliases

```nx
type Byte = u8
type Size = usize
type Result<T, E> = enum { Ok(T), Err(E) }
```

## Pointer Types

```nx
// Raw pointers (unsafe)
let ptr: *i32 = mem_alloc(4)
let mut_ptr: *mut i32 = mem_alloc(4)

// References (borrowed, safe)
let ref: &i32 = &value
let mut_ref: &mut i32 = &mut value
```

## Array & Slice Types

```nx
// Fixed-size array
let arr: [i32; 5] = [1, 2, 3, 4, 5]

// Slice (fat pointer: ptr + len)
let slice: []i32 = &arr[1..3]
```

## Struct Types

```nx
struct Vec2 {
    x: f32,
    y: f32,
}

struct Player {
    name: str,
    health: i32,
    pos: Vec2,
}
```

## Enum Types (Tagged Unions)

```nx
enum Option<T> {
    Some(T),
    None,
}

enum Result<T, E> {
    Ok(T),
    Err(E),
}

enum Event {
    KeyPress(char),
    MouseClick { x: i32, y: i32 },
    Quit,
}
```

## Function Types

```nx
// Function pointer
let callback: fn(i32) -> i32 = |x| x * 2

// Closure (future)
// let closure = |x| x + captured_var
```

## Type Inference

```nx
// Explicit
let x: i32 = 42

// Inferred
let x = 42        // i32 (default integer)
let y = 3.14      // f64 (default float)
let z = "hello"   // str
let arr = [1,2,3] // [i32; 3]
```

## Type Casting

```nx
// Safe casts (widening)
let a: i32 = 42
let b: i64 = a as i64
let c: f64 = a as f64

// Unsafe casts (narrowing, pointer)
let d: i32 = b as i32  // May truncate
let ptr = a as *i32    // Integer to pointer
```

## Generics (Partial)

```nx
// Type parameters
struct Vec<T> {
    ptr: *T,
    len: usize,
    cap: usize,
}

fn push<T>(vec: &mut Vec<T>, value: T) { ... }

// Constraints (future)
// fn sort<T: Ord>(slice: &mut [T]) { ... }
```

## Size & Alignment

```nx
sizeof(i32)      // 4
alignof(i32)     // 4
sizeof(Vec2)     // 8
alignof(Vec2)    // 4
offsetof(Vec2, y) // 4
```

## Zero Values

```nx
let a: i32 = 0        // or i32::zero()
let b: f64 = 0.0
let c: bool = false
let d: *i32 = null
let e: &i32 = null    // References can be null (unsafe)
let f: []i32 = []     // Empty slice
```

## Type Operators

```nx
// Type of expression
typeof(42)           // i32
typeof(3.14)         // f64

// Type equality
typeid(i32) == typeid(i32)  // true
```