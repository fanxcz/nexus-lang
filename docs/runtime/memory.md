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

    defer mem_free(buffer)
    mem_set(buffer, 0, 32)
    print("buffer initialized")
}
```

Register `defer mem_free(handle)` immediately after a successful allocation.
The deferred call captures the handle at registration time and releases it when
the lexical block exits, including on `return`, `break` or `continue`. This
avoids missing an explicit free on an ordinary control-flow exit. Deferred
cleanup currently requires the LLVM backend; it does not run after process
abort or other abrupt termination.

The pointer-like values are represented as integers at the language/runtime boundary. The runtime validates every positive-length `mem_read`, `mem_set` and `mem_copy` range against a live allocation returned by `mem_alloc`; a range must fit entirely inside one allocation. Invalid addresses, negative lengths and out-of-bounds ranges terminate with a runtime diagnostic. Zero-length writes/copies are no-ops, and `mem_read(0)` preserves the null-to-zero convention. Prefer ordinary language values unless low-level memory access is specifically required.

## Allocation lifetime

`mem_alloc` routes through the runtime's managed allocator. That allocator can run a conservative mark-and-sweep collection during later allocations; it scans the owning thread's active stack and saved register snapshot, and scans contents for eligible managed blocks. It is not a precise language-level ownership system, and a pointer stored as an integer or referenced only from another thread is not guaranteed to remain discoverable by the collector.

Use `mem_free(ptr)` for deterministic release, and never use a handle after freeing it. The managed C++ runtime checks that `mem_free` receives a currently registered allocation; an unknown or already-freed handle terminates with a runtime diagnostic. Range validation is performed while holding the allocation registry lock, so an explicit free or collection cannot race between the bounds check and the corresponding read/write/copy. This does not provide a language-level borrow checker or make an integer handle safe to use after it has been freed.

The semantic analyzer catches obvious use-after-free through direct
local aliases and supported user-function wrappers. It follows a single
managed-handle return origin through identity/forwarding functions, recognizes
functions that return a fresh allocation, and propagates direct `mem_free`
effects through helper chains. After a known release, subsequent uses in
`mem_read`, `mem_set` or `mem_copy` are rejected, as are known double frees.

This is not a complete ownership checker. Mixed/ambiguous return origins,
aggregate handles, pointer arithmetic, external calls, deferred cleanup and
loop-carried rebindings are not tracked. Runtime validation remains
authoritative for every checked memory operation.
