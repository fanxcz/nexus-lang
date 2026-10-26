# `std.option`

A concrete option type for values whose presence is not guaranteed. NEXUS does
not have generic types yet, so this module currently stores an `i64` payload.

## Type

```nx
enum Option { Some(i64), None }
```

## Functions

- `option_some(value: i64) -> Option`
- `option_none() -> Option`
- `option_is_some(value: Option) -> bool`
- `option_try_unwrap(value: Option, out: *i64) -> bool`: writes the payload and returns true for `Some`; returns false for `None` and leaves the output unchanged.
- `option_unwrap_or(value: Option, fallback: i64) -> i64`: returns the payload or an explicit fallback.
- `option_unwrap(value: Option) -> i64`: legacy helper that returns zero for `None`.

## Example

```nx
import std.option

fn main() {
    let count = option_some(12)
    let mut value: i64 = -1
    if option_try_unwrap(count, &value) {
        print(value)
    } else {
        print("No count")
    }

    print(option_unwrap_or(option_none(), 99))
}
```

Prefer `option_try_unwrap` or `option_unwrap_or` when zero is a valid payload
and a missing value must be distinguishable. The `?` propagation operator can
also unwrap a matching `Some(T)/None` enum on the LLVM backend when the
enclosing function returns that same enum type; see [Error Handling](../language/error-handling.md).
