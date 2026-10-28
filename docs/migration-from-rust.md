# Migrating from Rust to NEXUS

> This guide separates current behavior from the proposed systems-language direction. NEXUS does not yet implement Rust-equivalent memory safety, generic Result<T, E>, channels, tasks, async/await, arenas, proposed SIMD types or derive attributes. Do not assume proposed syntax compiles.

## 1. Comparison

| Topic | Rust | NEXUS today | Proposed NEXUS |
|---|---|---|---|
| Compiler | rustc/LLVM | C++20 compiler, LLVM/Clang path, built-in x86-64 backend for a defined subset | LLVM optimizing path and tested ARM64 support |
| Integer inference | Unsuffixed literals usually default to i32 | Current inferred integer type is i64 | Explicitly document let x = 5 as i64 |
| Mutability | Immutable by default; mut opts in | let immutable, let mut mutable | Preserve |
| Functions | Typed signatures and tail expressions | Typed params, optional result type, explicit return for non-void | Simple function syntax plus generics |
| Ownership | Ownership, borrow checker and lifetimes | Current pointers do not provide ownership safety | Affine moves, inferred regions and constrained capabilities |
| Cleanup | Drop and scope cleanup | defer has LLVM-backend limits; no complete owner/drop system | Drop glue plus explicit early drop(value) |
| Errors | Generic Result<T,E>, Option<T>, ? | Concrete enums/helpers; limited ? on LLVM | Generic Result/Option and propagation |
| Generics | Trait-bounded generics | Generic types/functions and traits are unavailable | Function generics first |
| Concurrency | Threads, channels, async ecosystem | No built-in chan<T>/spawn/async runtime | Owned messages, task handles and constrained async |
| SIMD | Compiler/library/target dependent | Proposed vector types are not built-ins | v2f/v3f/v4f/mat4 with LLVM lowering |
| Layout | repr(C), packed and alignment | Proposed packed/aligned attributes unavailable | Explicit ABI and cross-target tests |
| Serialization | Serde ecosystem | Narrow built-in JSON helpers | Generated codecs and schema validation |
| Packages | Cargo and registry | Source imports and local package artifacts | Simple imports plus pinned registry packages |
| Docs | rustdoc HTML | Basic Markdown from the configured entry source | Static HTML and cross-file API docs |
| Tests/bench | cargo test and benchmark ecosystem | nexus test and process-level nexus bench | Function-level benchmark annotations after runtime support |

## 2. Porting approach

1. Inventory unsafe blocks, generics, trait objects, escaping lifetimes, async tasks, shared mutable data, build scripts, native dependencies and serialization formats.
2. Port pure algorithms first, keeping explicit parameter/return types and explicit return statements.
3. Replace dependencies by capability, not by name. A same-named NEXUS helper does not prove identical semantics or platform support.
4. Current NEXUS lacks generic collections/error types. For prototypes, use documented typed integer/string lists and concrete enum helpers as temporary workarounds.
5. Isolate pointer/FFI boundaries. Current NEXUS does not validate Rust-style lifetimes; check nullability, alignment, ownership and ABI explicitly.
6. Port regression tests before optimizing. Benchmark the LLVM and native backends separately.
7. Keep proposed chan, spawn, arena, async/await, SIMD and derive syntax out of code expected to pass today's compiler.

## 3. Syntax translation

### Function

Rust:

~~~rust
fn add(a: i64, b: i64) -> i64 {
    a + b
}
~~~

Current NEXUS:

~~~nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}
~~~

### Bindings and loops

Rust:

~~~rust
let limit = 5;
let mut sum = 0;
for i in 0..limit {
    sum += i;
}
~~~

Current NEXUS:

~~~nx
let limit = 5
let mut sum: i64 = 0
for i in 0..limit {
    sum = sum + i
}
~~~

### Error enums

Rust can return Result<i64, ParseError>. Current NEXUS cannot express that generic type, but can express a concrete enum:

~~~nx
enum ParseResult {
    Ok(i64),
    Err(string)
}

fn parse_example() -> ParseResult {
    return ParseResult::Err("invalid input")
}
~~~

This is not a drop-in replacement: generic error conversion and propagation rules differ.

## 4. Future ownership/concurrency guidance

- Passing an owning value transfers it unless explicitly Copy.
- Clone only when an independent copy is needed.
- Keep read-only views inside their owner's region; return owned data when lifetime behavior would be unclear.
- Send owned messages through chan<T> rather than sharing mutable structures across workers.
- Give workers disjoint entity ranges and return result messages; queue ECS structural changes for a safe simulation phase.
- Never let arena values escape the arena or survive await.
- Keep FFI/raw pointer operations explicitly unsafe and separately reviewed.

These are future-edition rules, not guarantees provided by current NEXUS pointers.

## 5. Common pitfalls

- Assuming Rust syntax in the proposal compiles. Generic types/traits/impl blocks, async/await and channels are not current features.
- Assuming ? handles all error types. Current propagation is limited to compatible concrete enum variants on LLVM.
- Assuming all programs use the same backend. The built-in native backend has a narrower x86-64 subset; other code may need LLVM/Clang.
- Assuming JSON helpers are Serde or that nexus doc builds a website. Both APIs are much narrower.
- Assuming package publish uploads to a hosted registry. Current publishing creates local artifacts.
- Translating lifetimes into raw pointers, which removes checks rather than preserving safety.
- Optimizing before profiling. Record backend, target, hardware and repetitions, and separate startup from throughput.

## 6. Verification checklist

- Run nexus check and compile using the intended backend/target.
- Port tests for empty inputs, overflow, invalid indexes, allocation/resource failures and malformed data.
- Run ctest --test-dir build --output-on-failure when building from source.
- Review each pointer/FFI boundary for lifetime, alignment, ABI and ownership.
- Benchmark repeated runs on the same machine, and record unsupported behavior explicitly.

See the [systems-language proposal](language-spec.md), [current language reference](language/index.md), [tooling commands](tooling/commands.md) and [build guide](development/building.md).
