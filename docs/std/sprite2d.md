# `std.sprite2d`

Texture loading, dimensions, drawing and sprite-sheet cells.

- `sprite2d_load_texture(path) -> i64`: returns 0 on failure.
- `sprite2d_unload_texture(texture)`
- `sprite2d_texture_width(texture) -> i64` / `sprite2d_texture_height(texture) -> i64`
- `sprite2d_draw_texture(texture, x, y, width, height)`
- `sprite2d_draw_frame(texture, x, y, width, height, frame, frame_width, frame_height, columns)`

The draw wrappers use a white tint. Use `std.animation2d` to calculate frame indexes. The loader's supported formats depend on optional runtime image libraries; check for handle 0 and query `gfx_error()` when loading fails.
