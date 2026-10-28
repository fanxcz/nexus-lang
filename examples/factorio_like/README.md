# NEXUS Factorio-like simulation baseline

This is a compiler-compatible starting point for data-oriented factory simulation.

## Run

From the repository root:

~~~sh
nexus check examples/factorio_like/main.nx
nexus run examples/factorio_like/main.nx
~~~

Expected summary: 10,000 entities, 1,000 conveyor records, four logical worker partitions, 30 ticks and 270,000 item updates.

## Files

- `main.nx` — entry point.
- `ecs.nx` — structure-of-arrays world using the current integer-list API.
- `factory_loop.nx` — simulation loop with non-overlapping logical ranges.
- `config.nx` — source-level defaults composed at check/build time.

## Important limits

The four worker ranges run sequentially. The current compiler does not implement `chan<T>`, `spawn`, or a safe task runtime, so this sample does not create four threads. It uses integer grid coordinates instead of proposed SIMD types.

Imported NEXUS source is composed during compilation; `config.nx` cannot be hot-reloaded by a running process. Runtime config watching/module reload is future work, not implemented here. See [the systems-language proposal](../../docs/language-spec.md) for the concurrency, SIMD, arena, hot-reload and ECS acceptance criteria.

This is a deterministic baseline, not a Factorio clone or a published performance benchmark.
