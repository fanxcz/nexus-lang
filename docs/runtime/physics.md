# Physics (2D)

Box2D-style 2D physics engine.

## World

```nx
import runtime.physics

fn main() {
    // Create world with gravity
    let world = phys_world_create(vec2(0, -9.81))
    defer phys_world_destroy(world)
    
    // Configure
    phys_world_set_velocity_iterations(world, 8)
    phys_world_set_position_iterations(world, 3)
}
```

## Bodies

```nx
// Body types
enum BodyType {
    Static,    // Never moves (walls, ground)
    Kinematic, // Moves by velocity (platforms)
    Dynamic,   // Affected by forces (player, boxes)
}

// Create body
let body_def = phys_body_def {
    type: BodyType::Dynamic,
    position: vec2(0, 10),
    angle: 0.0,
    linear_damping: 0.1,
    angular_damping: 0.0,
    fixed_rotation: false,
    bullet: false,  // Continuous collision for fast objects
}

let body = phys_body_create(world, body_def)
```

## Shapes

```nx
// Circle
let circle = phys_shape_circle(1.0)  // radius

// Box (rectangle)
let box = phys_shape_box(2.0, 1.0)  // half-width, half-height

// Polygon
let vertices = [vec2(-1,-1), vec2(1,-1), vec2(0,1)]
let poly = phys_shape_polygon(vertices)

// Edge (for ground)
let edge = phys_shape_edge(vec2(-10,0), vec2(10,0))
```

## Fixtures

```nx
// Attach shape to body with properties
let fixture_def = phys_fixture_def {
    shape: circle,
    density: 1.0,        // Mass per area (0 for static)
    friction: 0.3,       // Surface friction
    restitution: 0.5,    // Bounciness (0-1)
    is_sensor: false,    // Trigger only, no collision response
    filter: phys_filter {
        category_bits: 0x0001,
        mask_bits: 0xFFFF,
        group_index: 0,
    },
}

let fixture = phys_fixture_create(body, fixture_def)
```

## Forces & Impulses

```nx
// Apply force at center (accumulates, applied each step)
phys_body_apply_force(body, vec2(10, 0))

// Apply force at world point (creates torque)
phys_body_apply_force_at_point(body, vec2(0, 50), vec2(1, 2))

// Apply impulse (instant velocity change)
phys_body_apply_impulse(body, vec2(0, 100))

// Apply torque
phys_body_apply_torque(body, 5.0)

// Set velocity directly
phys_body_set_linear_velocity(body, vec2(10, 0))
phys_body_set_angular_velocity(body, 2.0)
```

## Step Simulation

```nx
fn fixed_update(world: *World, dt: f32) {
    // Fixed timestep recommended
    const TIME_STEP = 1.0 / 60.0
    phys_world_step(world, TIME_STEP)
}
```

## Collision Callbacks

```nx
// Set contact listener
phys_world_set_contact_listener(world, {
    fn begin_contact(contact: *Contact) {
        let a = phys_contact_get_fixture_a(contact)
        let b = phys_contact_get_fixture_b(contact)
        // Handle collision start
    }
    
    fn end_contact(contact: *Contact) {
        // Handle collision end
    }
    
    fn pre_solve(contact: *Contact, old_manifold: *Manifold) {
        // Modify contact before solving
    }
    
    fn post_solve(contact: *Contact, impulse: *ContactImpulse) {
        // Get impulse data after solving
    }
})
```

## Raycasting

```nx
let hit = phys_world_raycast(world, vec2(0,0), vec2(10,0), { fixture ->
    // Filter callback
    true  // Hit all
})

if hit.hit {
    println("Hit at: {}, normal: {}", hit.point, hit.normal)
    println("Fraction: {}", hit.fraction)  // 0-1 along ray
}
```

## Querying

```nx
// AABB query
let bodies = phys_world_query_aabb(world, aabb_min, aabb_max)

// Point query
let fixtures = phys_world_query_point(world, vec2(100, 200))

// Circle query
let fixtures = phys_world_query_circle(world, vec2(100, 200), 50)
```

## Debug Draw

```nx
phys_world_set_debug_draw(world, {
    fn draw_polygon(vertices: []vec2, count: i32, color: u32) { ... }
    fn draw_circle(center: vec2, radius: f32, color: u32) { ... }
    fn draw_segment(p1: vec2, p2: vec2, color: u32) { ... }
    fn draw_transform(xf: Transform) { ... }
})

// In render
phys_world_debug_draw(world)
```

## Complete Example

```nx
import runtime.physics
import runtime.gfx

fn main() {
    let world = phys_world_create(vec2(0, -9.81))
    defer phys_world_destroy(world)
    
    // Ground (static)
    let ground = phys_body_create(world, phys_body_def {
        type: BodyType::Static,
        position: vec2(0, 0),
    })
    phys_fixture_create(ground, phys_fixture_def {
        shape: phys_shape_edge(vec2(-20, 0), vec2(20, 0)),
        friction: 0.6,
    })
    
    // Dynamic box
    let box = phys_body_create(world, phys_body_def {
        type: BodyType::Dynamic,
        position: vec2(0, 10),
    })
    phys_fixture_create(box, phys_fixture_def {
        shape: phys_shape_box(1, 1),
        density: 1.0,
        friction: 0.3,
        restitution: 0.5,
    })
    
    gfx_init(800, 600, "Physics")
    defer gfx_quit()
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()
        phys_world_step(world, 1.0/60.0)
        
        let pos = phys_body_get_position(box)
        
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        // Draw box
        gfx_draw_rect(pos.x - 32, pos.y - 32, 64, 64, 0xffffff)
        
        // Debug draw
        phys_world_debug_draw(world)
        
        gfx_end_frame()
    }
}
```