# `std.camera2d`

Small wrappers for the active runtime camera:

- `camera2d_set(x, y, zoom)`
- `camera2d_reset()`

Camera transformations depend on the graphics backend/path in use. The tilemap debug renderer also accepts an explicit viewport and translates world positions to screen coordinates; avoid applying both translations to the same draw calls.
