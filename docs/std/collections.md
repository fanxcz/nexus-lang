# `std.collections`

This module provides checked wrappers around the runtime's opaque growable lists.
It currently has separate lists for `i64` values and strings; it does **not**
implement generic `Vec<T>`, slices, iterators, or user-defined generic types.

## Integer lists

| Function | Meaning |
|---|---|
| `int_list_new() -> i64` | Create an empty list handle |
| `int_list_reserve(handle, capacity) -> bool` | Preallocate capacity; negative capacities fail |
| `int_list_push(handle, value) -> bool` | Append a value, returning false for invalid handles/allocation failure |
| `int_list_insert(handle, index, value) -> bool` | Insert at an index from 0 through the current length |
| `int_list_try_get(handle, index, &out) -> bool` | Read an existing element into a mutable `i64` |
| `int_list_set(handle, index, value) -> bool` | Replace an existing element |
| `int_list_remove(handle, index, &out) -> bool` | Remove an element and return the removed value through `out` |
| `int_list_try_pop(handle, &out) -> bool` | Pop the last element; false when empty |
| `int_list_len(handle) -> i64` | Number of elements (zero for an invalid handle) |
| `int_list_clear(handle) -> bool` | Remove all elements but keep the handle |
| `int_list_free(handle) -> bool` | Release the list handle |

## String lists

The same operations are available as `string_list_*` functions, with
`string` values and an output parameter of type `*string`. For example,
`string_list_try_pop(handle, &value)` returns false for an empty list and
does not overwrite the output value on failure.

## Example

```nx
import std.collections

fn main() {
    let values = int_list_new()
    defer int_list_dispose(values)

    if !int_list_push(values, 10) {
        print("Could not append")
        return
    }
    if !int_list_insert(values, 0, 5) {
        print("Could not insert")
        return
    }

    let mut first: i64 = 0
    if int_list_try_get(values, 0, &first) {
        print(first)
    }
}
```

Use `int_list_dispose` or `string_list_dispose` with `defer` to release
handles on every normal exit path. Handles are opaque integers: do not perform
arithmetic on them, use them after freeing them, or assume they survive process
restart. The checked mutation/read helpers return false for invalid handles,
invalid indexes, empty pops, and allocation failures; their output parameters
are unchanged when an operation fails.

These functions currently require the LLVM backend. The built-in x86-64 native
backend is a documented subset and does not yet provide parity for these list
operations.