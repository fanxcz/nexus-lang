# `std.memory`

The current `stdlib/memory.nx` module provides two thin wrappers:

- `memory_alloc(bytes: i64) -> i64`
- `memory_free(ptr: i64)`

These map to the runtime's `mem_alloc` and `mem_free` built-ins. Allocation returns an integer address-like handle; 0 indicates that no usable block was produced.

## Example

```nx
import std.memory

fn main() {
    let buffer = memory_alloc(64)
    if buffer == 0 {
        print("Allocation failed")
        return
    }

    // This is a raw address-like handle, not a typed byte array.
    memory_free(buffer)
}
```

The module does not provide arena, pool or frame allocators, allocation statistics, type-safe boxes, slices or automatic lifetime management. The runtime separately exposes low-level `mem_set` and `mem_copy`; those operations require valid handles and byte counts. Treat this as unsafe low-level functionality.
