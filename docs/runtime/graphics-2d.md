# Graphics 2D

The current 2D API uses `gfx_begin`, `gfx_end`, `gfx_clear`, `gfx_rect`, `gfx_circle`, `gfx_line` and `gfx_text`. Older names such as `gfx_begin_frame`, `gfx_delta_time` and `gfx_draw_rect` are not current functions.

## Working example

```nx
fn main() {
    let ok = gfx_init(800, 480, "NEXUS 2D")
    if !ok {
        print("Graphics unavailable: " + gfx_error())
        return
    }

    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") { break }

        gfx_begin()
        gfx_clear(0.025, 0.035, 0.07, 1.0)
        gfx_rect(40.0, 50.0, 220.0, 100.0, 0.1, 0.25, 0.5, 1.0)
        gfx_circle(360.0, 100.0, 38.0, 0.2, 0.8, 1.0, 1.0)
        gfx_line(40.0, 200.0, 420.0, 260.0, 3.0, 1.0, 0.7, 0.25, 1.0)
        gfx_text(40.0, 310.0, 2.0, "Press ESC to quit", 0.9, 0.95, 1.0, 1.0)
        gfx_end()
    }
    gfx_shutdown()
}
```

## Drawing calls

- `gfx_clear(r, g, b, a)`
- `gfx_rect(x, y, width, height, r, g, b, a)`
- `gfx_circle(x, y, radius, r, g, b, a)`
- `gfx_line(x1, y1, x2, y2, width, r, g, b, a)`
- `gfx_text(x, y, scale, text, r, g, b, a)`

The 2D origin is the upper-left corner. Colors are RGBA floats, not a packed integer.

## Window and input helpers

`gfx_set_title(text)`, `gfx_fullscreen(enabled)`, `gfx_maximize()`, `gfx_restore()`, `gfx_is_fullscreen()`, `gfx_width()`, `gfx_height()`, `gfx_vsync(enabled)`, `gfx_backend()`, `gfx_error()`, and `gfx_set_ui_resolution(width, height)` are available. For mouse state and elapsed time see [Input](input.md).

## Texture, font and canvas helpers

- Textures: `gfx_texture_load`, `gfx_texture_draw`, `gfx_texture_draw_frame`, `gfx_texture_filter`, `gfx_texture_unload`, `gfx_texture_width`, `gfx_texture_height`.
- Fonts: `gfx_font_load`, `gfx_text_font`, `gfx_font_unload`; depends on the optional TTF runtime library.
- CPU canvas: `gfx_canvas_create`, `gfx_canvas_brush`, `gfx_canvas_line`, `gfx_canvas_rect`, `gfx_canvas_fill`, `gfx_canvas_checkpoint`, `gfx_canvas_undo`, `gfx_canvas_redo`, `gfx_canvas_save_bmp`, `gfx_canvas_load_bmp`, `gfx_canvas_draw`, `gfx_canvas_destroy`, and dimension helpers.

Texture loaders return ID 0 on failure. The canvas supports BMP load/save, not a general set of image formats. See the working [Paint template](../templates/paint.md).
