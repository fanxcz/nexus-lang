# Input and Events

Input is provided by built-in `gfx_*` functions; there is no separate `runtime.input` module in the current runtime.

## Held keyboard and mouse state

```nx
while !gfx_should_close() {
    gfx_poll()
    if gfx_key_down("ESC") { break }
    if gfx_key_down("W") {
        // move up
    }
    if gfx_mouse_down(1) {
        // left mouse button is held
    }
    let mouse_x = gfx_mouse_x()
    let mouse_y = gfx_mouse_y()
}
```

`gfx_key_down(name)` checks held key state. Examples use names such as `"W"`, `"A"`, `"S"`, `"D"`, `"ENTER"`, `"ESC"` and `"F11"`. `gfx_mouse_down(1)` checks left, `2` middle, and `3` right mouse button. These functions report held state, not just a one-frame press.

`gfx_mouse_clicked(button)` is the edge-triggered companion: it returns `true` only on the frame the button went down. Use it for buttons and cards so a single press cannot fire an action twice.

```nx
if gfx_mouse_clicked(1) {
    // one-shot click
}
```

`gfx_mouse_x()` / `gfx_mouse_y()` return the current pointer position, so they can be combined with the click helper for hit-testing:

```nx
fn clicked_in(x: f64, y: f64, w: f64, h: f64) -> bool {
    let mx = gfx_mouse_x()
    let my = gfx_mouse_y()
    return gfx_mouse_clicked(1) && mx >= x && mx <= x + w && my >= y && my <= y + h
}
```

For convenience, `std.engine2d` wraps these as `engine2d_mouse_x()`, `engine2d_mouse_y()`, `engine2d_mouse_down(button)`, `engine2d_mouse_clicked(button)`, `engine2d_mouse_over(x, y, w, h)` and `engine2d_mouse_clicked_in(x, y, w, h)`.

Escape also sets the window-closed flag, so `gfx_should_close()` returns `true` after the user presses it. Loops that already test `gfx_should_close()` therefore quit on Escape without an explicit `gfx_key_down("ESC")` check; keep the explicit check if you want to run cleanup before leaving the loop.

## Events

Call `gfx_poll()` once per frame. Helpers describe the most recently processed event: `gfx_event_is(name)`, `gfx_event_type()`, `gfx_event_key()`, `gfx_event_mouse_button()`, `gfx_event_x()` and `gfx_event_y()`.

Names recognized by `gfx_event_is` include `quit`, `key_down`, `key_up`, `mouse_move`, `mouse_down`, `mouse_up` and `window`.

## Timing and coordinates

`gfx_dt()` returns seconds elapsed since the previous call. Cache it once per frame. `gfx_mouse_x()` / `gfx_mouse_y()` return current pointer coordinates mapped through the active UI resolution; configure the virtual space with `gfx_set_ui_resolution(width, height)`.

## Limits

The current API provides keyboard/mouse state and SDL-derived window events. It does not currently document controller state, touch callbacks, per-key just-pressed APIs or an on-screen keyboard.
