# 3D and voxel game development status

NEXUS is being extended toward native voxel-game development. The language and
compiler have useful foundations, but the current runtime is an experimental
graphics layer rather than a finished game engine.

## Available today

- Fixed-width signed/unsigned integers and `f32`/`f64` with width-aware LLVM code generation.
- Fixed-size arrays, read-only slices (`[]T`), and lexical exclusive mutable slices (`mut []T`) with bounds checks.
- Structs, enums, pointers, functions, `defer`, and the existing memory/runtime helpers.
- `vec2`/`vec3` math helpers, SDL-based input/windowing and 2D drawing.
- An experimental 3D API using compatibility OpenGL 2.1, including a perspective camera, grid/cube helpers and a basic OBJ loader.

## Current 3D limitations

The current 3D renderer is not suitable as the final renderer for a large voxel world. It does not yet provide a modern shader pipeline, vertex/index buffer mesh ownership, greedy chunk meshing, GPU-side chunk batching, frustum culling, LOD, or a resource streaming system. The existing cube helper uses immediate-mode OpenGL and should be treated as a demonstration primitive, not as a per-voxel rendering strategy.

The current runtime also does not expose a general-purpose worker/job system or a complete 3D archetype ECS. There is no built-in Perlin/Simplex terrain generator or versioned compressed chunk save format.

## Recommended implementation sequence

1. **Memory and collections:** explicit arena/pool allocators and predictable allocation for frame/chunk hot paths; typed growable collections after generic functions are implemented.
2. **Math and camera:** `Vec4`, `Mat4`, quaternions, transforms and frustum primitives.
3. **3D rendering backend:** mesh/VBO/IBO abstraction, shaders, material/texture-atlas path, batching and deterministic resource lifetime.
4. **Voxel data model:** chunk coordinates, compact block IDs, palette/compressed storage and versioned binary serialization.
5. **Meshing:** greedy face merging, transparent/opaque passes, neighbor-border updates and asynchronous mesh rebuilds.
6. **World generation:** seeded noise, biome parameters and deterministic regeneration.
7. **Scheduling and visibility:** bounded worker pool, job queues, cancellation, frustum culling and distance-based LOD.
8. **Integration tests and examples:** a playable chunk streaming sample, benchmark scenes and memory/CPU/GPU profiling.

## Backend selection

Use the default `--backend auto` for applications that use slices or the newer
numeric types. The current native x86-64 backend supports a smaller documented
subset; it does not compile slice-based programs. The LLVM backend is the current
path for these language features.

## Status policy

This page describes shipped capabilities separately from roadmap items. A roadmap
item is not considered implemented until it has a documented API, regression
tests, backend support and a working example.
