# UI Helpers

The runtime offers immediate drawing widgets. They render into the current frame; they are not a separate retained GUI system, and do not include text fields, layout containers, menu systems or a widget tree.

## Available widgets

- `ui_panel(x, y, width, height, r, g, b, a)`
- `ui_label(x, y, text, scale)`
- `ui_button(x, y, width, height, label)` returns whether clicked.
- `ui_checkbox(x, y, label, checked)` returns the resulting boolean.
- `ui_slider(x, y, width, height, value)` returns a normalized value from 0.0 to 1.0.
- `ui_progress(x, y, width, height, value, r, g, b, a)` draws a progress bar.

Call widgets after event polling and between `gfx_begin()` and `gfx_end()`. Keep values in your own variables:

```nx
fn main() {
    let ok = gfx_init(800, 480, "NEXUS UI")
    if !ok { print(gfx_error()); return }

    let mut enabled = false
    let mut volume = 0.5
    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") { break }

        gfx_begin()
        gfx_clear(0.025, 0.035, 0.07, 1.0)
        ui_panel(24.0, 24.0, 420.0, 300.0, 0.06, 0.08, 0.14, 1.0)
        ui_label(48.0, 48.0, "SETTINGS", 2.0)
        enabled = ui_checkbox(48.0, 100.0, "Enabled", enabled)
        volume = ui_slider(48.0, 160.0, 300.0, 28.0, volume)
        ui_progress(48.0, 220.0, 300.0, 18.0, volume, 0.2, 0.7, 1.0, 1.0)
        gfx_end()
    }
    gfx_shutdown()
}
```

For complex user interfaces, draw using the 2D primitives and implement layout/application state explicitly.
