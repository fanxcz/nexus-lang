# Runtime API

Game and application runtime for graphics, audio, input, ECS, and more.

## In This Section

| Module | Description |
|--------|-------------|
| [Overview](overview.md) | Runtime overview |
| [Memory](memory.md) | Allocation and arenas |
| [Graphics 2D](graphics-2d.md) | SDL2 2D rendering |
| [Graphics 3D](graphics-3d.md) | OpenGL 3D rendering |
| [Audio](audio.md) | Sound and music |
| [Input](input.md) | Keyboard, mouse, gamepad |
| [ECS](ecs.md) | Entity Component System |
| [Scenes](scenes.md) | Scene management |
| [Physics](physics.md) | 2D physics |
| [Particles](particles.md) | Particle systems |
| [UI](ui.md) | Immediate-mode UI |

## Quick Example

```nx
import runtime.gfx
import runtime.input

fn main() {
    gfx_init(800, 600, "My Game")
    defer gfx_quit()
    
    let mut x = 400.0
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()
        
        if input_key_down(KEY_LEFT)  { x -= 300.0 * dt }
        if input_key_down(KEY_RIGHT) { x += 300.0 * dt }
        
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        gfx_draw_rect(x, 300, 50, 50, 0xffffff)
        gfx_end_frame()
    }
}
```