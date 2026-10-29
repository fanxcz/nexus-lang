# Memory Model

NEXUS does not have a language-level ownership or borrowing system, and it does not guarantee that every allocation is managed by a garbage collector. The C++ runtime does include a conservative tracing collector for allocations routed through its managed allocator, which is used by selected string/file helpers and by `mem_alloc`.

The collector runs per owning thread. It scans that thread's active stack and a saved register snapshot, then scans the contents of eligible managed blocks. Because this is a conservative implementation, these details are runtime behavior—not a guarantee that arbitrary pointer values, integer-encoded handles, or references held only by another thread will keep an allocation alive. Do not treat it as a borrow checker or as proof that pointer-heavy code is safe.

## Practical guidance

- Keep ownership of raw allocations explicit and call `mem_free` when finished; never use a handle after it has been freed.
- Avoid using pointers after the object or backing allocation has been released.
- Treat runtime objects as main-thread-owned unless a specific API documents thread safety.
- Use `defer` for calls returning `void`; the LLVM backend executes deferred calls in LIFO order on normal scope exit, `return`, `break`, and `continue`. Its current built-in x86-64 backend rejects `defer`, and process aborts do not run language cleanup.

## Current limitations

The language does not enforce a complete borrow-checking or ownership system. Runtime collection is conservative and is not a substitute for deterministic resource release, valid pointer handling, or testing. Do not assume that Rust-style memory safety guarantees are provided merely because the syntax has references and explicit types.

Related topics: [runtime memory APIs](../runtime/memory.md) and [language overview](../language/overview.md).
