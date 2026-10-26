# `std.result`

A concrete result type for an `i64` success value or a string error. NEXUS does
not have generic types yet, so this module does not provide `Result<T, E>`.

## Type

```nx
enum Result { Ok(i64), Err(string) }
```

## Functions

- `result_ok(value: i64) -> Result`
- `result_err(message: string) -> Result`
- `result_is_ok(value: Result) -> bool`
- `result_try_unwrap(value: Result, out: *i64) -> bool`: writes the success payload and returns true for `Ok`; returns false for `Err` and leaves the output unchanged.
- `result_unwrap_or(value: Result, fallback: i64) -> i64`: returns the success payload or an explicit fallback.
- `result_try_error(value: Result, out: *string) -> bool`: copies the error message and returns true for `Err`; returns false for `Ok` and leaves the output unchanged.
- `result_unwrap(value: Result) -> i64`: legacy helper that returns zero for `Err`.

## Example

```nx
import std.result

fn main() {
    let result = result_err("configuration missing")
    let mut value: i64 = 0
    if result_try_unwrap(result, &value) {
        print(value)
    } else {
        let mut message = ""
        if result_try_error(result, &message) {
            print(message)
        }
    }
}
```

Prefer checked extraction over `result_unwrap`, because zero may be a valid
success value. The `?` operator supports matching `Ok(T)/Err` enums on the
LLVM backend only when the enclosing function returns that same result enum.
Cross-type propagation and generic `Result<T, E>` are not implemented; see
[Error Handling](../language/error-handling.md).
