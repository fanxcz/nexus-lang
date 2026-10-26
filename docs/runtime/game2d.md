# NEXUS modular 2D game libraries

Game functionality is split into small modules under `stdlib/`. Each can be imported independently, and modules with dependencies state them explicitly.

| Module | Responsibility |
|---|---|
| [`std.engine2d`](../std/engine2d.md) | Window, frame timing, keyboard and primitive drawing |
| [`std.math`](../std/math.md) | Clamping, interpolation, floor, approach, min/max and sign |
| [`std.tilemap`](../std/tilemap.md) | Grid storage and checked cell access |
| [`std.tilemap_render`](../std/tilemap_render.md) | Viewport-aware colored tile drawing |
| [`std.collision2d`](../std/collision2d.md) | AABB/circle tests and axis-separated movement against tilemaps |
| [`std.animation2d`](../std/animation2d.md) | Time-based sprite-sheet frame selection |
| [`std.sprite2d`](../std/sprite2d.md) | Texture and sprite-sheet drawing |
| [`std.camera2d`](../std/camera2d.md) | Active runtime camera wrappers |
| [`std.audio2d`](../std/audio2d.md) | Optional audio loading/playback |

## Platformer example

The [`examples/game2d_platformer.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/game2d_platformer.nx) imports the core frame, math, tilemap, rendering and collision modules. It runs without external image/audio assets.

```bash
nexus check examples/game2d_platformer.nx
nexus run examples/game2d_platformer.nx
```

Controls: A/D or arrow keys to move, SPACE to jump, ESC to exit.

## Limits

This is a modular set of helpers on top of the shipped runtime, not yet a full game engine. Tilemaps are in-memory fixed grids; there is no Tiled import/editor, per-tile collision metadata, texture-atlas management, dedicated tilemap destruction or rigid-body world. Collision movement uses axis-separated substeps and assumes an entity starts outside solid tiles. Graphics and sound still depend on optional runtime libraries.
