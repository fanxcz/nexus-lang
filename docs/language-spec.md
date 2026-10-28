# NEXUS Systems Language Specification — Proposal

> **Status: design proposal, not a claim about the current compiler.** This document describes a staged target for a future NEXUS edition. The current compiler does not implement generic types, channels, spawn blocks, async/await, arenas, proposed SIMD vector types, or derive attributes. Current source and backend limits remain authoritative until a feature lands with implementation and tests.

## 1. Goals and compatibility

NEXUS should become a practical native systems language for simulation-heavy games and engine work without Rust's learning curve. The design targets LLVM IR and native code, predictable ownership, message-passing concurrency, data-oriented ECS and memory APIs, and honest multi-platform support.

Use three status labels: **Implemented** (accepted by the current compiler and tested), **Experimental** (implemented with documented restrictions), and **Proposed** (design only). Do not imply that a proposal is executable. Existing NEXUS source remains the compatibility baseline; introduce new ownership rules under an edition/version rather than silently changing the meaning of legacy pointers. NEXUS today does not provide Rust-equivalent memory safety.

## 2. Core syntax

### Bindings and functions

~~~nx
let count = 5          // inferred i64
let mut total: i64 = 0
const max_items: i64 = 10000

fn add(a: i64, b: i64) -> i64 {
    return a + b
}

fn log_ready() {       // returns void if no type is written
    print("ready")
}
~~~

Rules: let is immutable; let mut is mutable. Unsuffixed integer literals infer i64 unless an expected type says otherwise. A function without a result type returns void. Non-void functions return a value on every path. Generic functions use `fn name<T>(...)` and infer type arguments where unambiguous. Initial generics should focus on functions and monomorphization, without requiring user-written traits/impl blocks.

### Result values

~~~nx
enum FileError {
    NotFound,
    PermissionDenied,
    InvalidData
}

fn load_level(path: string) -> Result<Level, FileError> {
    let bytes = read_file(path)?
    return parse_level(bytes)
}
~~~

The target standard library has generic Result<T, E> and Option<T>. The postfix ? propagates a compatible error value. Error conversion must be explicit or follow a small documented rule; error detail must not be lost. Generic enums are not current NEXUS functionality.

## 3. Memory model — simpler than Rust's general borrow checker

### Single-owner values and moves

Every non-Copy value has one owning binding. Assignment and function calls move ownership unless a type is explicitly Copy. Scalars may be Copy. Struct/array copyability must be explicit and permitted only when all fields allow it. Cloning an owner is never implicit.

~~~nx
let packet = make_packet()
let outgoing = packet       // moves packet
// use(packet)              // proposed error: already moved
send(channel, outgoing)     // receiver becomes owner
~~~

### Deterministic cleanup

Compiler-generated drop glue runs in reverse construction order at scope exit, including early returns and error propagation. `drop(value)` releases a value early; that binding becomes unusable. Drop glue must be non-panicking. Resource handles, tasks, channels and arena regions require deterministic cleanup. Abrupt process termination is not a cleanup guarantee.

### Regions and arenas

~~~nx
arena frame {
    let scratch = arena_alloc<Particle>(frame, 4096)
    simulate_particles(scratch)
} // allocations in this arena are released together
~~~

Arena blocks bulk-free allocations at scope exit. Arena-backed values cannot escape via a return value, longer-lived struct, global, or task that outlives the arena. Child arenas cannot outlive parents. Implement fixed/growable arenas, alignment-aware allocation, overflow checks and allocation-failure results. Non-trivial arena values must be destroyed in reverse construction order or prove they need no drop.

### Safety without a full borrow checker

Single-owner moves alone cannot guarantee memory safety if arbitrary aliases and raw pointers remain possible. A smaller region/capability model is still required:

- Safe code cannot dereference raw pointers. Raw pointer operations and FFI require an explicit unsafe block.
- A read-only view cannot outlive its owner or cross await unless stored in an owned/static region.
- Mutation goes through the owner or a statically unique capability. Safe code cannot create two active mutable capabilities to the same allocation.
- Cross-task/channel values must be owned or pass structural send-safety checks.

These rules are compile-time checks, even if the feature is intentionally much smaller than Rust's borrow/lifetime system. Stage them: move/use-after-move; region escape; unique mutation; task-send safety; explicit FFI boundaries. Do not call the result memory-safe until negative tests prove these invariants.

## 4. Concurrency

### Channels

~~~nx
struct Job {
    entity_start: i64,
    entity_end: i64,
    tick: i64
}

fn worker(jobs: chan<Job>, results: chan<Job>) {
    while true {
        let job = recv(jobs)?
        process_range(job)
        send(results, job)?
    }
}
~~~

chan<T> is a typed message queue, preferably bounded by default for back-pressure. send transfers ownership; recv returns an owned message or typed channel-closed error. Send/receive define synchronization and happens-before. A sender cannot retain a mutable alias to the sent payload. Closing wakes blocked tasks; queue ordering, cancellation, fairness and shutdown semantics must be documented and tested. The first runtime must not expose unsynchronized shared mutation.

### Tasks and async

~~~nx
let task = spawn {
    worker(job_channel)
}
let result = await task.join()

async fn load_manifest(url: string) -> Result<string, NetError> {
    let response = await http_get_async(url)?
    return response.body
}
~~~

spawn returns TaskHandle and moves captured owned values into the task. Join returns its result; detaching is explicit. async functions compile to state machines and await is a suspension point. Stack views and non-static arena values cannot survive suspension. Cancellation runs cleanup. These features require an executor, wakeups, task-state codegen and error/cancellation behavior, not just parser keywords.

## 5. Game-dev types, layout and serialization

### SIMD and math

- `v2f`: two IEEE-754 float32 lanes.
- `v3f`: three logical float32 lanes with documented ABI/padding.
- `v4f`: four float32 lanes.
- `mat4`: 4×4 float32 matrix with defined storage and multiplication convention.

Support vector arithmetic, dot, length, normalize and selected matrix operations. Initially overload operators only for these built-in math types. LLVM should lower to target-independent vector IR, selecting SSE/AVX on x86-64 or NEON on ARM64 where supported, with scalar fallback. Define floating-point tolerances; do not silently enable fast-math behavior that changes NaN/Inf semantics. The built-in native backend should reject unsupported operations until it has tests.

### Layout attributes

~~~nx
#[aligned(16)]
struct Particle {
    position: v4f,
    velocity: v4f,
    flags: i64
}

#[packed]
struct WireHeader {
    version: i64,
    kind: i64
}
~~~

aligned(N) validates target-supported power-of-two alignment. packed accesses require safe unaligned loads/stores or a diagnostic, and cannot create normally aligned references to unaligned fields. Parser, semantic layout, debug info, FFI ABI, serializers and both code generators must agree.

### Serialization

~~~nx
#[derive(Serialize, Deserialize)]
struct Recipe {
    name: string,
    input_count: i64,
    output_count: i64
}
~~~

Derives generate concrete codecs. JSON/binary decoding validates field types, numeric ranges, buffer lengths, nesting limits and schema versions. Binary format defines byte order and is independent of in-memory padding. Invalid/untrusted input returns a typed error and cannot trigger unchecked allocation sizes.

## 6. Standard-library target

Use the existing `import std.NAME` convention. Each module needs API docs, negative/error tests and backend/target notes.

| Module | Target contract | Important constraint |
|---|---|---|
| std.ecs | Archetype ECS, generation-counted entity IDs, component registration, cached queries, deterministic iteration | Reject stale IDs; control structural mutation |
| std.math | Scalar math and v2f/v3f/v4f/mat4 | SIMD plus scalar fallback |
| std.io | Checked streaming reads/writes, zero-copy views, memory-mapped files | Views cannot outlive their mapping handle |
| std.json | Streaming parser/writer and typed decoding | Bounded depth/size; no external crate dependency |
| std.arena | Fixed/growable arenas and aligned allocation | Compile-time region escape checks |
| std.sync | Channels, task handles, atomics and cancellation | No unsynchronized shared mutation |
| std.time | Monotonic clock, timers and frame deltas | Distinguish wall-clock from monotonic time |

The target ECS groups entities by component signature, stores components in contiguous columns, caches query plans and batches structural changes through command buffers. Systems declare read/write access so the scheduler can reject conflicts or run independent work concurrently. Entity IDs contain generation counters. Test at least 100,000 create/destroy operations, stale-ID rejection, query-cache invalidation and deterministic 10,000-entity simulation for 1,000 ticks.

## 7. Compiler and tooling architecture

Implement a pipeline, not just keywords:

1. Lexer/token model: add tokens and keywords with compatibility and identifier tests.
2. Parser/AST: generic declarations/type arguments, arena blocks, spawn blocks, async functions, channels, attributes and error diagnostics.
3. Semantic analysis: generic substitutions, move states, drop scopes, region containment, layout and send-safety.
4. Typed HIR or equivalent: represent drops, regions, await states, channel/task ownership and vector operations before codegen.
5. Runtime: arenas, channels, executor/task handles, cancellation, cleanup and serialization.
6. LLVM backend: monomorphization, async state machines, target layouts and vector IR; run the LLVM verifier.
7. Built-in native backend: preserve the currently documented subset and report unsupported features clearly; add support only after ABI/codegen tests.
8. CLI: templates, package manifests, tests, docs and benchmarks.
9. CI: cross-platform tests, sanitizers and automatic example execution.

Current source locations are `src/compiler/frontend/lexer/`, `src/compiler/frontend/parser/`, `src/compiler/ast/`, `src/compiler/semantic/`, `src/compiler/backend/codegen/`, `src/compiler/backend/native/`, `src/compiler/builtins/`, `src/compiler/format/` and `src/compiler/driver/commands/`. The older path `src/compiler/driver/semantic/` does not match the current tree. Formatter and quality commands should share the parser/token model instead of maintaining independent keyword lists.

### CLI/tooling targets

| Command | Target behavior | Current status |
|---|---|---|
| nexus new <name> --template game | Multi-file project with ECS, config, tests and assets | A game template exists; this proposal adds a standalone example |
| nexus build / nexus run | Target/backend selection and precise unsupported-feature diagnostics | Implemented with backend limits |
| nexus add <url> | Pin/validate a registry dependency | Proposed alias; current command is nexus pkg add and focuses on local vendoring |
| nexus publish | Upload immutable packages to a real registry | No hosted registry today; current package publish creates local artifacts |
| nexus doc | API pages and static HTML | Current command generates basic Markdown from the entry file |
| nexus test | Tests and optional function-level benchmarks | Project tests exist; bench currently measures whole-process runs |
| nexus fmt | Parse-aware formatting and no-write check | Formatter exists but is currently modest |

Do not describe planned tooling as implemented before the service/runtime exists and CI verifies it.

## 8. Twelve-week implementation roadmap

This schedule assumes dedicated engineering effort; it is a plan, not a guarantee. A phase cannot graduate without acceptance tests.

| Week | Phase | Exit gate |
|---|---|---|
| 1 | Baseline/design freeze | Compatibility suite and feature-status matrix |
| 2 | Typed HIR/layout | Type IDs, generic substitution design and canonical layout |
| 3 | Generics MVP | Generic functions, inference, monomorphization and diagnostics |
| 4 | Ownership MVP | Move analysis, use-after-move errors, deterministic drop scopes |
| 5 | Regions/arenas | Alignment, exhaustion and region-escape rejection |
| 6 | Channels | Bounded queues, transfer semantics, close/shutdown stress |
| 7 | Tasks/executor | spawn, TaskHandle, join/cancel and worker shutdown |
| 8 | Async MVP | State-machine lowering, await lifetime rules and cleanup |
| 9 | SIMD/layout | Vector lowering, packed/aligned ABI tests and scalar parity |
| 10 | ECS/serialization | Archetypes, derives, input limits and deterministic benchmarks |
| 11 | Cross-platform/tooling | Windows/Linux/macOS; ARM64 build spike; docs/fmt/template integration |
| 12 | Release candidate | Audit, fuzz/sanitizer/performance results, migration docs and support matrix |

If safety invariants fail, defer the feature instead of relabeling it stable to meet the schedule.

## 9. Test plan

**Language:** infer let x = 5 as i64; reject wrong annotations, use-after-move and double drop; verify drop order on normal/early exits and ? propagation; reject arena escape via return/struct/global/task/await; test generic inference/recursion; cover Result success/error conversion; verify packed/aligned layouts on x86-64 and ARM64.

**Concurrency:** bounded queue back-pressure, ordering, close while blocked, allocation failure, send ownership, raw pointer/region rejection, four non-overlapping worker ranges against a single-thread checksum, cancellation, repeated join, worker errors and cleanup-on-cancel. Use ThreadSanitizer where available; ASan/UBSan do not prove absence of data races.

**ECS/runtime:** stale generation IDs, query invalidation, command-buffer order, conflict detection and deterministic scheduling. Fuzz malformed generics, nested arenas, attributes, async state and channel types.

**Backends/performance:** verify generated LLVM IR; assert vector IR in SIMD kernels, compare scalar/vector results within tolerance, and test scalar fallback. Run builds/tests on Ubuntu, macOS and Windows; add ARM64 ABI/build coverage before claiming support. Benchmark ECS, factory updates, arenas, channels and JSON separately from process startup; record machine, OS, target, backend, workload and repeated-run statistics. Test supported examples in CI.

## 10. File plan and completion criteria

Foundation in this change: the language and migration specifications, semantic-safety checks, backend-parity tests, and runtime regression tests. Examples should demonstrate language features generically or use original NEXUS projects, not reproduce third-party games.

Subsequent source work: lexer under `src/compiler/frontend/lexer/`; parser/AST under `src/compiler/frontend/parser/` and `src/compiler/ast/`; semantic analysis under `src/compiler/semantic/`; LLVM under `src/compiler/backend/codegen/`; native backend under `src/compiler/backend/native/`; builtins under `src/compiler/builtins/`; runtime under `src/runtime/`; standard library under `stdlib/`; tests under `tests/`; build gates in `CMakeLists.txt` and `.github/workflows/{build,sanitizers}.yml`.

The example in this change is a compiler-compatible baseline only. Do not claim Rust-level memory safety or high-performance concurrency until ownership/region rules, task/channel shutdown, x86-64/ARM64 codegen, bounded runtime errors and repeatable benchmark results are implemented and tested.
