# Particles

The runtime exposes helpers for creating, updating and drawing simple 2D particles. It does not include a complete emitter editor, GPU buffer API, collision responses, curves, or preset system.

## Example

```nx
fn main() {
    let ok = gfx_init(960, 540, "NEXUS Particles")
    if !ok { print(gfx_error()); return }

    let mut burst = 0
    while !gfx_should_close() {
        gfx_poll()
        let dt = gfx_dt()
        if burst < 120 {
            let p = gfx_particle_create(480.0, 270.0, 120.0, -80.0,
                                        1.2, 5.0, 0.2, 0.75, 1.0, 1.0)
            if gfx_particle_alive(p) { burst = burst + 1 }
        }
        gfx_particles_update(dt)
        gfx_begin()
        gfx_clear(0.02, 0.03, 0.06, 1.0)
        gfx_particles_draw()
        gfx_end()
    }
    gfx_particles_clear()
    gfx_shutdown()
}
```

Calls are `gfx_particle_create(x, y, vx, vy, lifetimeSeconds, size, r, g, b, a)`, `gfx_particle_alive(id)`, `gfx_particles_update(dt)`, `gfx_particles_draw()`, `gfx_particle_destroy(id)` and `gfx_particles_clear()`. Use the current frame delta for updates.
