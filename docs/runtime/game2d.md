# NEXUS 2D game helpers

`stdlib/engine2d.nx` is a convenience layer over existing runtime calls; it is not a separate rendering backend. It provides consistent names for frame setup, delta-time clamping, keyboard input, rectangles/circles/text, textures, sprite-sheet frames, camera, AABB overlap and basic sound playback.

The module intentionally keeps game state in the application. Current runtime limitations remain: AABB is overlap detection rather than a rigid-body solver, camera behavior depends on the active graphics path, audio and image formats depend on optional runtime libraries, and no tilemap editor or general asset pipeline is provided yet.

Example:

```nx
import std.engine2d

fn main() {
    if !engine2d_init(800, 450, "NEXUS 2D") {
        print(gfx_error())
        return
    }

    while !engine2d_should_close() {
        engine2d_begin_frame(0.03, 0.05, 0.10, 1.0)
        engine2d_draw_rect(80.0, 90.0, 64.0, 64.0, 0.95, 0.25, 0.18, 1.0)
        engine2d_draw_text(24.0, 24.0, 1.5, "NEXUS 2D", 1.0, 1.0, 1.0, 1.0)
        engine2d_end_frame()
    }

    engine2d_shutdown()
}
```

The self-contained platformer example at `examples/game2d_platformer.nx` demonstrates movement, jump/gravity, simple platform landing, frame-time clamping and drawing without external assets.
