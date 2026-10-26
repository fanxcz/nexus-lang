# `std.camera2d`

Small wrappers for the active runtime camera:

- `camera2d_set(x, y, zoom)`
- `camera2d_reset()`
- `camera2d_follow(current, target, speed, dt) -> Vec2`: frame-delta-driven follow step.
- `camera2d_world_to_screen(world, position, zoom, viewport_width, viewport_height) -> Vec2`
- `camera2d_screen_to_world(screen, position, zoom, viewport_width, viewport_height) -> Vec2`
- `camera2d_clamp(position, world_width, world_height, viewport_width, viewport_height, zoom) -> Vec2`

The coordinate helpers use a centered viewport; non-positive zoom falls back to 1.0. The follow helper uses a clamped `speed * dt` interpolation amount. Pass frame delta time to avoid frame-rate-dependent movement. Call `camera2d_set(result.x, result.y, zoom)` to apply a returned position to the runtime camera.

Camera transformations depend on the graphics backend/path in use. The tilemap debug renderer also accepts an explicit viewport and translates world positions to screen coordinates; avoid applying both translations to the same draw calls.
