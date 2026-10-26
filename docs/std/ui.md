# std.ui

Immediate-mode UI helpers for native applications. The module adds rectangle
geometry, basic layout helpers, and readable wrappers around the runtime's
existing drawing widgets. It does not create a retained widget tree or manage
application state for you.

## Layout geometry

- ui2d_rect(x, y, width, height) -> UIRect
- ui2d_right(rect), ui2d_bottom(rect), ui2d_center_x(rect), ui2d_center_y(rect)
- ui2d_contains(rect, x, y) -> bool
- ui2d_inset(rect, padding) -> UIRect
- ui2d_row_cell(parent, index, count, gap) -> UIRect
- ui2d_column_cell(parent, index, count, gap) -> UIRect
- ui2d_grid_cell(parent, column, row, columns, rows, gap) -> UIRect

Negative gaps/padding are treated as zero. Invalid indexes/counts return a zero-size rectangle. Layout functions do not mutate the parent rectangle.

## Widgets

- ui2d_panel(rect, r, g, b, a)
- ui2d_outline(rect, thickness, r, g, b, a)
- ui2d_label(x, y, text, scale)
- ui2d_button(rect, label) -> bool
- ui2d_checkbox(x, y, label, checked) -> bool
- ui2d_slider(rect, value) -> f64, normalized to [0, 1]
- ui2d_progress(rect, value, r, g, b, a)
- ui2d_separator(x1, y1, x2, y2, thickness, r, g, b, a)
- ui2d_text_field(rect, value, placeholder, max_bytes) -> string

Call widgets after polling events and between the runtime's begin/end drawing calls.
Keep the returned checkbox and slider values in your own mutable variables. The
API is immediate-mode: widgets draw every frame and process the current input
state. The text field supports single-line UTF-8 input while the graphics runtime
is active: click to focus, Backspace to remove one code point, and Enter or Tab
to finish editing. `max_bytes` is a byte limit (the runtime caps it at 8192
bytes), not a character count. Selection, caret movement, IME editing commands,
caller-supplied widget IDs, automatic focus cycling, and Escape-to-cancel are
not implemented. Retained widget trees, accessibility semantics, and native
file dialogs are also outside the current API.

## Example

The repository's examples/ui_playground.nx demonstrates a settings panel,
checkboxes, a slider, a progress bar, layout cells, and a clickable About panel.
Run it from the repository root after building the current compiler:

    nexus check examples/ui_playground.nx
    nexus run examples/ui_playground.nx --backend llvm


## Text field example

```nx
import std.ui

fn main() {
    let ok = gfx_init(800, 480, "NEXUS text field")
    if !ok { print(gfx_error()); return }
    let mut name = ""
    while !gfx_should_close() {
        gfx_poll()
        gfx_begin()
        gfx_clear(0.025, 0.035, 0.065, 1.0)
        name = ui2d_text_field(ui2d_rect(80.0, 120.0, 360.0, 42.0),
                               name, "Your name", 128)
        ui2d_label(80.0, 190.0, "Name: " + name, 1.2)
        gfx_end()
    }
    gfx_shutdown()
}
```
