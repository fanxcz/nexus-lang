# UI Helpers

For rectangle geometry and row/column/grid layout helpers, see [std.ui](../std/ui.md). It wraps these immediate-mode built-ins under the `ui2d_*` names to avoid name collisions.

The runtime offers immediate drawing widgets. They render into the current frame; they are not a separate retained GUI system. Alongside buttons and sliders, the runtime includes a basic single-line UTF-8 text field; retained layout containers, menus and a widget tree are not provided.

## Available widgets

- `ui_panel(x, y, width, height, r, g, b, a)`
- `ui_label(x, y, text, scale)`
- `ui_button(x, y, width, height, label)` returns whether clicked.
- `ui_checkbox(x, y, label, checked)` returns the resulting boolean.
- `ui_slider(x, y, width, height, value)` returns a normalized value from 0.0 to 1.0.
- `ui_progress(x, y, width, height, value, r, g, b, a)` draws a progress bar.
- `ui_text_field(x, y, width, height, current, placeholder, max_bytes)` returns the current or edited UTF-8 string.

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

For `ui_text_field`, click to focus, type UTF-8 text, Backspace removes one code point, and Enter or Tab ends editing. `max_bytes` is limited to 1–8192; a non-positive value makes the field read-only. The control is identified by its rectangle; it has no caret/selection editing, Escape cancellation, explicit widget IDs or full multi-field Tab traversal yet.

For complex user interfaces, draw using the 2D primitives and implement layout/application state explicitly.
