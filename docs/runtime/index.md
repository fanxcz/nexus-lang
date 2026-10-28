# Runtime API

The runtime exposes built-in functions to NEXUS programs. This reference describes the current implementation in `src/runtime/`, which is one translation unit (`runtime.cpp`) assembled from small topic files (`core/`, `graphics/`, `audio/`, `io/`, `data/`, `strings/`, `collections/`, `ui/`, `game/`, `math/`, `memory/`, `net/`, `crypto/`) rather than a planned game-engine API.

## Pages

| Topic | What is implemented |
|---|---|
| [Overview](overview.md) | Frame loop, backend and limits |
| [2D Game Helpers and Tilemaps](game2d.md) | Tilemap cells, rendering, AABB collision and axis-separated movement |
| [Graphics 2D](graphics-2d.md) | Drawing primitives, textures, fonts and CPU raster canvas |
| [Graphics 3D](graphics-3d.md) | Cubes, grid and experimental model loading |
| [Input](input.md) | Held and edge-triggered keyboard/mouse state and window events |
| [Audio](audio.md) | Sound/music helpers and audio availability |
| [UI](ui.md) | Immediate drawing widgets |
| [ECS](ecs.md) | Simple built-in entity helpers |
| [Particles](particles.md) | 2D particle creation and drawing |
| [Physics](physics.md) | Rectangle/circle overlap tests |
| [Scenes](scenes.md) | ECS entity snapshots |
| [Memory](memory.md) | Raw memory operations |

The runtime is still evolving. It does not currently expose a full rigid-body physics world, generic ECS queries, shader API, complete JSON parser, touch/gamepad API, a scene transition manager or retained GUI framework.
