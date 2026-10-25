# Runtime Overview

The current runtime is made of built-in `gfx_*`, `ecs_*`, `ui_*`, `scene_*`, `physics_*`, `mem_*`, audio and file/network functions. Runtime functions are called directly; the current language does not require imports such as `runtime.gfx`, `runtime.input` or `runtime.audio`.

## Typical frame loop

```nx
fn main() {
    let ok = gfx_init(800, 480, "NEXUS Runtime")
    if !ok {
        print("Graphics unavailable: " + gfx_error())
        return
    }

    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") { break }

        gfx_begin()
        gfx_clear(0.025, 0.035, 0.07, 1.0)
        gfx_text(30.0, 30.0, 2.0, "NEXUS Runtime", 0.55, 0.85, 1.0, 1.0)
        gfx_end()
    }

    gfx_shutdown()
}
```

Call `gfx_poll()` each frame, update state, draw between `gfx_begin()` and `gfx_end()`, then shut the runtime down before returning.

## Timing, dimensions and backend

- `gfx_dt()`: elapsed seconds since the previous call; call once per frame and reuse the value.
- `gfx_width()`, `gfx_height()`: current window dimensions.
- `gfx_set_ui_resolution(width, height)`: optional virtual drawing coordinate space.
- `gfx_backend()`: backend name string.
- `gfx_error()`: most recent runtime error message.

Drawing colours are supplied as separate red, green, blue and alpha floating-point values, normally in the range 0.0–1.0.

## API scope

2D rendering includes rectangles, circles, lines, text, textures, custom fonts and a CPU-side raster canvas. The 3D API currently provides a camera setup, grid, coloured cubes and experimental model loading. Input covers keyboard/mouse state and the most recent event. Audio is dynamically loaded and may be unavailable. ECS/UI/particles/physics/scenes are small helper systems rather than complete engine subsystems.

See [Graphics 2D](graphics-2d.md), [Input](input.md), [Audio](audio.md), [ECS](ecs.md), [Physics](physics.md) and [Scenes](scenes.md) for exact APIs and limitations.

For a small convenience layer over the current APIs, see [2D game helpers](game2d.md) and the self-contained [platformer example](../../examples/game2d_platformer.nx).
