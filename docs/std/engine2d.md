# `std.engine2d`

Core convenience wrappers for the existing runtime window, frame loop, input and primitive drawing calls.

- `engine2d_init(width, height, title) -> bool`
- `engine2d_shutdown()`
- `engine2d_begin_frame(red, green, blue, alpha)`: polls events, begins drawing and clears the frame.
- `engine2d_end_frame()`
- `engine2d_delta_time() -> f64`: clamps the runtime delta time to 0–0.05 seconds.
- `engine2d_key_down(key) -> bool`, `engine2d_should_close() -> bool`
- `engine2d_draw_rect(...)`, `engine2d_draw_circle(...)`, `engine2d_draw_text(...)`

Textures/sprites, camera, sound, tilemap storage/rendering and collision are in their own modules. These wrappers are a convenience API, not a separate graphics backend.
