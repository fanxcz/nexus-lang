# ECS Helpers

The runtime has a compact set of entity helper functions for simple games. It is not a general-purpose ECS world with arbitrary component registration or component queries. Built-in entities store position, velocity, size, colour and optional texture ID.

## Example

```nx
fn main() {
    let ok = gfx_init(960, 540, "NEXUS ECS")
    if !ok { print(gfx_error()); return }

    let player = ecs_create()
    ecs_set_position(player, 160.0, 220.0)
    ecs_set_velocity(player, 80.0, 0.0)
    ecs_set_size(player, 52.0, 52.0)
    ecs_set_color(player, 0.2, 0.8, 1.0, 1.0)

    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") { break }
        ecs_update(gfx_dt())
        gfx_begin()
        gfx_clear(0.02, 0.03, 0.06, 1.0)
        ecs_draw()
        gfx_end()
    }
    ecs_destroy(player)
    gfx_shutdown()
}
```

## Helpers

- `ecs_create()`, `ecs_destroy(id)`, `ecs_alive(id)`
- `ecs_set_position(id, x, y)`, `ecs_set_velocity(id, x, y)`, `ecs_set_size(id, width, height)`
- `ecs_set_color(id, r, g, b, a)`, `ecs_set_texture(id, textureId)`
- `ecs_x(id)`, `ecs_y(id)`, `ecs_count()`
- `ecs_update(dt)`, `ecs_draw()`, `ecs_collides(a, b)`

Colour components use normalized floating-point RGBA values. Use your own program data for health, inventory, AI state and custom components; the runtime does not provide a generic entity query API.
