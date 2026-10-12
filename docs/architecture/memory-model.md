# Memory Model

NEXUS 1.1.0 generates native code and does not provide a tracing garbage collector. Programs can use stack values, pointers, and runtime allocation helpers; ownership abstractions such as fully implemented smart pointers are still planned.

## Practical guidance

- Keep ownership of allocations explicit and pair each allocation with the matching release operation.
- Avoid using pointers after the object or backing allocation has been released.
- Treat runtime objects as main-thread-owned unless a specific API documents thread safety.
- Use `defer` where supported to keep cleanup close to resource acquisition.

## Current limitations

The language does not yet enforce a complete borrow-checking or ownership system. Pointer-heavy code therefore requires careful testing. Do not assume that Rust-style memory safety guarantees are provided merely because the syntax has references and explicit types.

Related topics: [runtime memory APIs](../runtime/memory.md) and [language overview](../language/overview.md).
