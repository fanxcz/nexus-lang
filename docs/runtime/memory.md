# Memory Helpers

The runtime provides raw memory helpers. The current API does not expose arenas, pools, frame allocators, allocation statistics or lifetime-tracking objects.

- `mem_alloc(bytes)` returns an integer address-like handle, or 0 when no block is produced.
- `mem_free(ptr)` releases a block created by the helper.
- `mem_set(ptr, value, bytes)` fills the memory with the low byte of `value`.
- `mem_copy(destination, source, bytes)` copies a sequence of bytes.

```nx
fn main() {
    let buffer = mem_alloc(32)
    if buffer == 0 {
        print("Allocation failed")
        return
    }
    mem_set(buffer, 0, 32)
    mem_free(buffer)
}
```

The pointer-like values are represented as integers at the language/runtime boundary. Only pass valid current handles and safe sizes; do not pass arbitrary or stale values. Prefer ordinary language values unless low-level memory access is specifically required.
