# Type System

The compiler has a static type system with inference for initialized bindings. Built-in numeric types are signed `i8`, `i16`, `i32`, `i64`, unsigned `u8`, `u16`, `u32`, `u64`, and floating-point `f32`, `f64`. The other built-in types are `bool`, `string` and `void`. `int`, `isize` and `usize` remain aliases of `i64` for compatibility; `float` remains an alias of `f64`. User-defined structs, enums with one supported scalar payload per variant, pointers and fixed-size arrays are also supported.

## Built-in types

| Type | Meaning |
|---|---|
| `i8`, `i16`, `i32`, `i64` | Signed 8-, 16-, 32- and 64-bit integers |
| `u8`, `u16`, `u32`, `u64` | Unsigned 8-, 16-, 32- and 64-bit integers |
| `f32`, `f64` | IEEE-754 32- and 64-bit floating-point numbers |
| `bool` | `true` or `false` |
| `string` | String value |
| `void` | No return value |
| `*T` | Pointer to a value of type `T` |
| `[T; N]` | Fixed-size array of `N` values of type `T` |
| `[]T` | Read-only non-owning slice of values of type `T` (pointer plus element count) |
| `mut []T` | Exclusive mutable slice of values of type `T`; must be created explicitly from mutable array storage |

Types such as `char`, `str`, `Option<T>` and `Result<T, E>` are not implemented as built-in generic types by the current parser. The integer aliases `int`, `isize` and `usize` still mean `i64`; they do not adopt the host pointer width.

## Slices and borrowing

A read-only slice `[]T` is a non-owning view represented as a data pointer and an
element count. `array[start..end]` creates a half-open view; the upper bound is
not included. Slices can be read, passed to functions, returned when they borrow
from a parameter, and queried with `len`. They cannot be written through.

Mutation requires an explicit exclusive borrow with `mut []T`:

```nx
fn set_first(values: mut []i64) {
    values[0] = 42
}

fn main() {
    let mut data: [i64; 3] = [1, 2, 3]
    {
        let mut view: mut []i64 = mut data[1..3]
        view[0] = 20
        view[1] += 10
    }
    // The borrow ends at the closing brace.
    print(data[1])
}
```

The `mut []T` qualifier is not interchangeable with `[]T`. A mutable slice must
be created with `mut array[start..end]`, and its owner must be a named array
declared with `let mut`. While that exclusive borrow is active, the owner cannot
be accessed and another shared or mutable slice of the same owner cannot be
created. A mutable slice cannot be returned or stored in a struct. Reborrowing
an existing slice and disjoint-field/range analysis are not implemented yet.

Mutable slices are supported by the LLVM backend. The built-in x86-64 native
backend does not support slices; `--backend auto` selects LLVM when a slice is
used. This is a lexical, conservative borrow checker rather than a complete
Rust-style lifetime and alias analysis system. Raw pointers and FFI remain
outside these guarantees.

## Numeric operations and literal typing

A numeric literal defaults to `i64` or `f64` when no other type is known. In a typed binding, function argument, return value, struct field, fixed-array element or enum payload, a literal is checked against that expected numeric type before being accepted. This allows the full `u64` literal range, including values above `i64::MAX`, when the expected type is `u64`.

Different numeric types do not implicitly convert. An `i32` variable cannot be assigned to an `i64` variable without an explicit conversion facility; a literal can take the expected type only when it is representable in that type. Unsigned types reject negative literals. `f32` literals must be finite and fit the finite `f32` range.

Integer arithmetic uses the selected width. Add/subtract/multiply use the underlying fixed-width bit pattern (wrapping on overflow); division and remainder use signed operations for `iN` and unsigned operations for `uN`. Division by zero is a runtime error, as is signed minimum divided by `-1`. Comparisons use signed ordering for `iN` and unsigned ordering for `uN`. Right shift is arithmetic for signed integers and logical for unsigned integers; the shift count is masked to the width minus one. Floating-point arithmetic uses `float` operations for `f32` and `double` operations for `f64`.

The LLVM backend supports the fixed-width scalar types. The built-in x86-64 native backend still supports only its documented `i64`, `bool` and `string` subset; with `--backend auto`, programs that use the new numeric types require the LLVM path.

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

Slice types and expressions are executable on the LLVM backend. A slice is a non-owning view represented as `{ ptr, i64 }`: a pointer to the first element and the number of elements. For example:

```nx
fn sum(values: []i64) -> i64 {
    let mut total: i64 = 0
    for i in 0..len(values) {
        total += values[i]
    }
    return total
}

fn identity(values: []i64) -> []i64 {
    return values
}

fn main() {
    let values = [10, 20, 30, 40]
    let middle: []i64 = values[1..3]
    let view = identity(middle)
    print(len(view))
    print(sum(view))
}
```

Slice ranges use a half-open interval: the start is included and the end is excluded, matching `for` range semantics. Both bounds are required and must have type `i64`. `len` accepts a string, fixed array, or slice; it returns the string length in bytes or the array/slice element count. Slicing an array or another slice is supported; temporary array expressions are rejected because the view must refer to named storage. Empty ranges are valid when `start == end` and the bounds are within the source length.

The LLVM backend checks dynamic slice ranges at runtime (`0 <= start <= end <= len`) and checks every dynamic slice index before loading its element. Slice indexing is read-only in this stage; indexed assignment still requires a fixed array, and the type system has no separate mutable-slice qualifier. Slice values can be passed to and returned from functions by value. The built-in native backend does not support slices, so `--backend auto` selects LLVM when a program uses them.

Slice values do not own their storage. The borrow-origin analysis tracks the array or slice binding from which a view was derived: returning a view into a local array is rejected, while returning a view borrowed from a function parameter is supported. This is a conservative lifetime check, not a complete borrow checker. Slices in struct fields and arrays of slices remain unsupported. Dynamically sized generic vectors are not currently provided by this type system. The `std.collections` module separately provides growable lists for `i64` values and strings as opaque runtime handles.

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

Struct fields and enum variants are statically checked. Enum variants may be unit variants or carry one payload of an integer type, `f32`, `f64`, `bool`, `string`, or pointer. Payload matching uses a binding pattern such as `Payload::Value(value)`. Generic enum payload types, multiple payload fields, structs/arrays/nested enums as payloads, and exhaustiveness checking for integer domains are not implemented. The LLVM backend represents enum values as a tag plus one 64-bit payload slot; scalar payloads are packed into that slot. The built-in native backend does not support enum values.

## Pointers

```nx
let mut value: i64 = 7
let ptr: *i64 = &value
let copy = *ptr
```

Pointers allow low-level access to values. The current type system does not provide safe references, lifetimes, ownership checking or `*mut T` syntax.

## Limits

Generic type parameters, tuple types, function types, trait bounds, user-defined type aliases, type-level operators such as `sizeof`/`alignof`, null pointer literals, and closure typing are not supported by the current type parser. Slices are supported on the LLVM backend but not by the built-in native backend. Use the concrete types and operations demonstrated in the [syntax guide](syntax.md) and the repository's regression programs.
