# Memory Model

NEXUS does not have a language-level ownership or borrowing system, and it does not guarantee that every allocation is managed by a garbage collector. The C++ runtime does include a conservative tracing collector for allocations routed through its managed allocator, which is used by selected string/file helpers and by `mem_alloc`.

The collector runs per owning thread. It scans that thread's active stack and a saved register snapshot, then scans the contents of eligible managed blocks. Because this is a conservative implementation, these details are runtime behavior—not a guarantee that arbitrary pointer values, integer-encoded handles, or references held only by another thread will keep an allocation alive. Do not treat it as a borrow checker or as proof that pointer-heavy code is safe.

## Practical guidance

- Keep ownership of raw allocations explicit and call `mem_free` when finished; never use a handle after it has been freed.
- Avoid using pointers after the object or backing allocation has been released.
- Treat runtime objects as main-thread-owned unless a specific API documents thread safety.
- Use `defer` for calls returning `void`; the LLVM backend executes deferred calls in LIFO order on normal scope exit, `return`, `break`, and `continue`. Its current built-in x86-64 backend rejects `defer`, and process aborts do not run language cleanup.

## Lexical-region and function-result checks

The semantic analyzer tracks the lexical origin of direct pointer borrows and
simple aliases. It rejects two known escape patterns: returning a pointer to
function-local storage, and storing a pointer from a nested block in a binding
whose scope outlives that block. Pointer provenance is also carried conservatively
through array, struct and enum initializers and through direct field/element
updates.

This is an incremental check, not a complete borrow checker. For NEXUS
functions, it conservatively follows pointer-bearing return values back to
pointer-bearing parameters, including aliases, nested helper calls and returned
aggregates. The summary is intentionally an over-approximation: a pointer
argument mentioned in a returned expression may be treated as an origin even
when a particular runtime path does not return it. External/FFI and builtin
calls remain opaque, and their returned pointers have no inferred lifetime.

The analyzer still does not track writes through already-dereferenced raw
pointers, prove unique mutable access, or prevent use-after-free of integer
allocation handles. Pointer-containing values crossing opaque external APIs
still require manual review.

## Current limitations

The language does not enforce a complete ownership or borrowing system. Runtime
collection is conservative and is not a substitute for deterministic resource
release, valid pointer handling, or testing. Do not assume that Rust-style
memory safety guarantees are provided merely because the syntax has references
and explicit types.

Related topics: [runtime memory APIs](../runtime/memory.md) and [language overview](../language/overview.md).
