# Type System

The current compiler has a small static type system with inference for initialized bindings. Supported built-in types are `i64`, `f64`, `bool`, `string` and `void`; `int` / `isize` / `usize` are currently aliases of the compiler's integer type, and `float` aliases its floating-point type. User-defined structs, enums with a single scalar payload per variant, pointers and fixed-size arrays are also supported.

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

## Pointer mutability rule

The current pointer type `*T` does not distinguish const pointers from mutable
pointers. Until const-qualified pointer types are implemented, the address-of
operator `&` therefore requires mutable storage rooted in a `let mut` binding.
For example, `let mut value = 10; let ptr = &value` is valid, while taking
`&value` when `value` was declared with plain `let` is rejected. This also
applies to fields and array elements owned by immutable bindings.

This restriction prevents a pointer from being used to bypass ordinary binding
immutability. It is not a full lifetime or aliasing checker: code using raw
pointers and FFI must still ensure that pointers remain valid and that accesses
are correctly synchronized.

## Numeric literals

Integer and floating-point literals may use underscores between digits to make
large values easier to read. Decimal floating-point literals also support
scientific notation; the exponent may include a sign.

```nx
const population: i64 = 1_000_000
let distance: f64 = 1.25e3
let tiny: f64 = 2.5E-4
let ratio: f64 = 12_345.6_7e-2
```

An underscore must appear between digits; repeated, leading, and trailing
separators are rejected. Scientific notation requires at least one exponent
digit, so `1e` and `2E+` are invalid literals.

## Numeric conversions and random integers

`to_i64(value)` rounds to the nearest integer, with halfway values rounded
away from zero. NaN converts to zero; positive/negative infinity and finite
values outside the signed 64-bit range saturate to `9223372036854775807` and
`-9223372036854775808`, respectively. This avoids undefined or
platform-dependent conversion behavior for out-of-range floating-point input.

`random_i64(minimum, maximum)` returns a pseudo-random integer in the
**inclusive** interval. Reversed bounds are normalized, so
`random_i64(10, 1)` behaves like `random_i64(1, 10)`. The generator is not
cryptographically secure and must not be used for tokens, passwords, or keys.

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

Slices and dynamically sized generic vectors are not currently supported by this type system. The `std.collections` module does provide separately typed growable lists for `i64` values and strings, but these are opaque runtime handles rather than generic value types.

## Structs and enums

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

Struct fields and enum variants are statically checked. Enum variants may be unit variants or carry one payload of type `i64`, `f64`, `bool`, `string`, or pointer. Payload matching uses a binding pattern such as `Payload::Value(value)`. Generic enum payload types, multiple payload fields, structs/arrays/nested enums as payloads, and exhaustiveness checking for `i64` domains are not implemented. The LLVM backend currently represents enum values as a tag plus one 64-bit payload slot; the built-in native backend has a smaller supported subset.

## Pointers

```nx
let mut value: i64 = 7
let ptr: *i64 = &value
let copy = *ptr
```

Pointers allow low-level access to values. The current type system does not provide safe references, lifetimes, ownership checking or `*mut T` syntax.

## Limits

Generic type parameters, tuple/slice types, function types, trait bounds, type aliases, type-level operators such as `sizeof`/ `alignof`, null pointer literals, and closure typing are not supported by the current type parser. Use the concrete types and operations demonstrated in the [syntax guide](syntax.md) and the repository's regression programs.
