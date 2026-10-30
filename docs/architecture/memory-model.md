# Memory Model

NEXUS does not have a language-level ownership or borrowing system, and it does not guarantee that every allocation is managed by a garbage collector. The C++ runtime does include a conservative tracing collector for allocations routed through its managed allocator, which is used by selected string/file helpers and by `mem_alloc`.

The collector runs per owning thread. It scans that thread's active stack and a saved register snapshot, then scans the contents of eligible managed blocks. Because this is a conservative implementation, these details are runtime behavior—not a guarantee that arbitrary pointer values, integer-encoded handles, or references held only by another thread will keep an allocation alive. Do not treat it as a borrow checker or as proof that pointer-heavy code is safe.

## Practical guidance

- Keep ownership of raw allocations explicit and call `mem_free` when finished; never use a handle after it has been freed. The managed C++ runtime rejects unknown and repeated frees, but it cannot make later dereferences of a stale integer handle safe.
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

The runtime now bounds `mem_read`, `mem_set`, and `mem_copy` to live
`mem_alloc` blocks and keeps the registry locked for the duration of each
operation. These checks protect only these managed-memory helpers; external
pointers passed through FFI remain outside the registry and require separate
validation.

The semantic analyzer also performs a narrow managed-allocation lifetime check.
It tracks a direct `mem_alloc` result through local aliases and known user
functions. Fixed-point summaries follow a single managed `i64` return origin
through identity/forwarding wrappers, identify functions that return a fresh
allocation, and propagate direct `mem_free` effects on arguments through
wrapper chains, including the case where a function returns a fresh
handle that it already freed internally. Known aliases are marked after
release, double frees are diagnosed, and freed state is merged conservatively
across branches. Direct `mem_read`, `mem_set`, and `mem_copy` arguments are
checked after their expressions are analyzed.

This remains a deliberately small analysis, not a general pointer/ownership
system. Mixed-origin return paths, aggregate handles, pointer arithmetic,
opaque external calls, deferred cleanup, and loop-carried rebindings are not
followed; ambiguous return summaries are discarded rather than guessed.
A control-flow join that changes allocation identity also drops the static
fact. Runtime checks remain authoritative, and the analyzer does not prove
unique mutable access.

## Current limitations

The language does not enforce a complete ownership or borrowing system. Runtime
collection is conservative and is not a substitute for deterministic resource
release, valid pointer handling, or testing. Do not assume that Rust-style
memory safety guarantees are provided merely because the syntax has references
and explicit types.

Related topics: [runtime memory APIs](../runtime/memory.md) and [language overview](../language/overview.md).
