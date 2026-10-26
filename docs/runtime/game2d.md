# NEXUS modular 2D game libraries

Game functionality is split into small modules under stdlib/. Each can be
imported independently, and modules with dependencies state them explicitly.
These are intended as a useful, tested 2D helper library, not a claim that NEXUS
already ships a full commercial game engine.

| Module | Responsibility |
|---|---|
| [std.engine2d](../std/engine2d.md) | Window, frame timing, keyboard/mouse state, primitive drawing and vsync |
| [std.math](../std/math.md) | Clamping, interpolation, floor, approach, min/max and sign |
| [std.vec2](../std/vec2.md) / [std.vec3](../std/vec3.md) | Vector math for movement and transforms |
| [std.tilemap](../std/tilemap.md) | Grid storage, checked access, fill and counting |
| [std.tilemap_render](../std/tilemap_render.md) | Viewport-aware colored tile drawing |
| [std.collision2d](../std/collision2d.md) | AABB/circle tests and axis-separated movement against tilemaps |
| [std.physics2d](../std/physics2d.md) | Gravity, jump speed, friction, vertical speed limits and ground checks |
| [std.animation2d](../std/animation2d.md) | Frame selection, duration/progress, easing and ping-pong curves |
| [std.sprite2d](../std/sprite2d.md) | Texture lifecycle, sprite sheets and tinted drawing |
| [std.camera2d](../std/camera2d.md) | Active runtime camera, follow, transforms and bounds |
| [std.audio2d](../std/audio2d.md) | Optional audio loading/playback, unload, volume, tone and diagnostics |

## Asset-free starter example

The [game2d_starter.nx](https://github.com/fanxcz/nexus-lang/blob/main/examples/game2d_starter.nx)
example demonstrates tilemap generation, collision, jump/gravity helpers,
camera follow, animated beacon effects and window/input management without
external art or audio files.

    nexus check examples/game2d_starter.nx
    nexus run examples/game2d_starter.nx --backend llvm

Controls: A/D or arrow keys to move, SPACE to jump, ESC to exit.

## Existing platformer example

The [game2d_platformer.nx](https://github.com/fanxcz/nexus-lang/blob/main/examples/game2d_platformer.nx)
imports the core frame, math, tilemap, rendering and collision modules.

    nexus check examples/game2d_platformer.nx
    nexus run examples/game2d_platformer.nx

## Limits

This is a modular set of helpers on top of the shipped runtime, not yet a full
game engine. Tilemaps are in-memory fixed grids; there is no Tiled importer or
editor, per-tile collision metadata, texture-atlas manager, or rigid-body world.
Collision movement uses axis-separated grid sweeps and assumes an entity starts
outside solid tiles. Graphics and sound still depend on optional runtime
libraries. The module contract and helper math have smoke/regression coverage;
graphics/audio behavior still requires manual testing on hosts with those
runtime libraries installed.
