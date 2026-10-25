# Type System

The current compiler has a small static type system with inference for initialized bindings. Supported built-in types are `i64`, `f64`, `bool`, `string` and `void`; `int` / `isize` / `usize` are currently aliases of the compiler's integer type, and `float` aliases its floating-point type. User-defined structs and unit enums, pointers and fixed-size arrays are also supported.

## Built-in types

| Type | Meaning |
|---|---|
| `i64` | Signed 64-bit integer |
| `f64` | 64-bit floating-point number |
| `bool` | `true` or `false` |
| `string` | String value |
| `void` | No return value |
| `*T` | Pointer to a value of type `T` |
| `[T; N]` | Fixed-size array of `N` values of type `T` |

Types such as `i8`, `u8`, `i16`, `i32`, `u32`, `u64`, `char`, `str`, `Option<T>` and `Result<T, E>` are not implemented as built-in types by the current parser.

## Inference and annotations

```nx
let count = 42
let name = "NEXUS"
let enabled = true

// Floating-point literals support scientific notation.
let distance: f64 = 1.25e3
let tiny: f64 = 2.5E-4

let mut total: i64 = 0
total = total + count
```

Bindings are immutable by default. Use `let mut` when the value needs reassignment. A `const` binding is also supported for values declared in the supported contexts.

## Fixed arrays

```nx
let values: [i64; 3] = [10, 20, 30]
let mut scores = [4, 8, 12]
scores[1] = 25
print(values[0])
print(scores[1])
```

Array types require a compile-time length, and the array literal must contain at least one element. The semantic checker verifies array element types and constant out-of-range indices; runtime handling of variable indices depends on the active backend.

Slices and dynamically sized generic vectors are not currently supported by this type system.

## Structs and unit enums

```nx
struct Player {
    health: i64
    speed: f64
}

enum Color {
    Red,
    Green,
    Blue
}

fn main() {
    let player = Player { health: 100, speed: 3.5 }
    let selected = Color::Green
    print(player.health)
}
```

Struct fields and enum variants are statically checked. Enums currently contain named unit variants only; payload variants and tagged-union pattern matching are not supported by the parser.

## Pointers

```nx
let mut value: i64 = 7
let ptr: *i64 = &value
let copy = *ptr
```

Pointers allow low-level access to values. The current type system does not provide safe references, lifetimes, ownership checking or `*mut T` syntax.

## Limits

Generic type parameters, tuple/slice types, function types, trait bounds, type aliases, type-level operators such as `sizeof`/ `alignof`, null pointer literals, and closure typing are not supported by the current type parser. Use the concrete types and operations demonstrated in the [syntax guide](syntax.md) and the repository's regression programs.
