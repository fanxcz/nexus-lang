# App Template

The `app` template creates a small native windowed application. It demonstrates graphics initialization, the event loop, keyboard/mouse polling, drawing primitives, text and shutdown.

## Create and run

```bash
nexus new MyApp --template app
cd MyApp
nexus check
nexus build
nexus run
```

## Included example

This is adapted from `examples/templates/app.nx`:

```nx
fn main() {
    let ok = gfx_init(1000, 640, "NEXUS App")
    if !ok {
        print("Graphics backend unavailable: " + gfx_error())
        return
    }

    let mut clicks: i64 = 0
    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") {
            break
        }
        if gfx_mouse_down(1) {
            clicks = clicks + 1
        }

        gfx_begin()
        gfx_clear(0.035, 0.045, 0.08, 1.0)
        gfx_rect(40.0, 40.0, 920.0, 110.0, 0.07, 0.12, 0.22, 1.0)
        gfx_text(70.0, 70.0, 3.0, "NEXUS APP", 0.55, 0.9, 1.0, 1.0)
        gfx_text(80.0, 410.0, 1.6, "CLICKS", 0.65, 0.75, 0.9, 1.0)
        gfx_text(190.0, 410.0, 1.6, str_i64(clicks), 1.0, 1.0, 1.0, 1.0)
        gfx_end()
    }
    gfx_shutdown()
}
```

## Extend it

Add your own state and update it after `gfx_poll()`. Draw between `gfx_begin()` and `gfx_end()`. Colors use normalized red, green, blue and alpha arguments; the current runtime does not use the older `gfx_begin_frame` / `gfx_draw_rect` API described in legacy drafts.

This template is a minimal example, not a bundled editor, asset browser or full application framework. See [Graphics 2D](../runtime/graphics-2d.md) and [Input](../runtime/input.md) for the implemented API.
