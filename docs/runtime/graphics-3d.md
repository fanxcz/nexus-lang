# Graphics 3D

The current 3D runtime exposes a small OpenGL-backed API for setting up a camera, drawing a grid and coloured cubes, and loading/drawing/unloading experimental models. It does not expose a general shader, uniform, arbitrary mesh or PBR material interface.

## Example

```nx
fn main() {
    let ok = gfx_init(960, 540, "NEXUS 3D")
    if !ok { print(gfx_error()); return }

    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") { break }
        gfx3d_begin(70.0, 0.1, 1000.0, 0.0, 2.4, 8.0, 20.0, 0.0, 0.0)
        gfx_clear(0.02, 0.025, 0.05, 1.0)
        gfx3d_grid(12, 1.0, 0.16, 0.22, 0.32, 1.0)
        gfx3d_cube(0.0, 1.0, 0.0, 1.5, 1.5, 1.5, 0.2, 0.75, 1.0, 1.0)
        gfx3d_end()
    }
    gfx_shutdown()
}
```

This follows the checked-in `examples/native_3d.nx` shape.

## Calls

`gfx3d_begin(fovDegrees, nearPlane, farPlane, cameraX, cameraY, cameraZ, pitch, yaw, roll)` sets up the 3D view. Draw with `gfx3d_grid(cells, spacing, r, g, b, a)` or `gfx3d_cube(x, y, z, sizeX, sizeY, sizeZ, r, g, b, a)`, then finish with `gfx3d_end()`.

Model helpers are `gfx3d_model_load(path)`, `gfx3d_model_draw(id, ...)` and `gfx3d_model_unload(id)`. The loader is experimental; the current API should not be assumed to support general glTF, custom shaders, skeletal animation or complex material definitions.
