# ECS (Entity Component System)

Data-oriented architecture for game logic.

## Core Concepts

| Concept | Description |
|---------|-------------|
| **Entity** | Unique ID (integer), no data |
| **Component** | Plain data struct, no behavior |
| **System** | Logic that queries entities with specific components |
| **World** | Container for all entities, components, systems |

## Basic Usage

```nx
import runtime.ecs

// 1. Define components
struct Position { x: f32, y: f32 }
struct Velocity { x: f32, y: f32 }
struct Sprite { texture: Texture, w: f32, h: f32 }
struct Player { }
struct Enemy { health: i32 }

// 2. Create world
let world = ecs_world_create()
defer ecs_world_destroy(world)

// 3. Register components
ecs_register_component(world, Position)
ecs_register_component(world, Velocity)
ecs_register_component(world, Sprite)
ecs_register_component(world, Player)
ecs_register_component(world, Enemy)

// 4. Create entities
let player = ecs_entity_create(world)
ecs_add(world, player, Position { x: 400.0, y: 300.0 })
ecs_add(world, player, Velocity { x: 0.0, y: 0.0 })
ecs_add(world, player, Sprite { tex: player_tex, w: 64, h: 64 })
ecs_add(world, player, Player {})

// 5. Query in systems
fn movement_system(world: *World, dt: f32) {
    let query = ecs_query(world, [Position, Velocity])
    for entity in query {
        let pos = ecs_get_mut(world, entity, Position)
        let vel = ecs_get(world, entity, Velocity)
        pos.x += vel.x * dt
        pos.y += vel.y * dt
    }
}

fn render_system(world: *World) {
    let query = ecs_query(world, [Position, Sprite])
    for entity in query {
        let pos = ecs_get(world, entity, Position)
        let spr = ecs_get(world, entity, Sprite)
        gfx_draw_texture(spr.texture, pos.x, pos.y)
    }
}

// 6. Run systems each frame
while !gfx_should_close() {
    let dt = gfx_delta_time()
    
    movement_system(world, dt)
    collision_system(world)
    ai_system(world, dt)
    
    gfx_begin_frame()
    gfx_clear(0x1a1a2e)
    render_system(world)
    gfx_end_frame()
}
```

## Queries

```nx
// Required components (AND)
let query = ecs_query(world, [Position, Velocity])

// Optional components
let query = ecs_query(world, [Position], [Velocity])  // Velocity optional

// Exclude components
let query = ecs_query(world, [Position], [], [Player])  // No Player

// Iterate
for entity in query {
    let pos = ecs_get_mut(world, entity, Position)
    // ...
}

// With optional
for entity in query {
    let pos = ecs_get_mut(world, entity, Position)
    if let Some(vel) = ecs_get_opt(world, entity, Velocity) {
        pos.x += vel.x
    }
}
```

## Archetypes (Internal)

Entities with same component set share an **archetype** for cache-friendly iteration.

```nx
// These entities share archetype: [Position, Velocity, Sprite]
// Different archetype: [Position, Sprite] (no Velocity)
```

## Commands (Deferred)

```nx
// Queue commands to run after current system
ecs_command_spawn(world, [Position {0,0}, Sprite{...}])
ecs_command_despawn(world, entity)
ecs_command_add(world, entity, Velocity{1,0})
ecs_command_remove(world, entity, Enemy)

// Flush at frame end
ecs_commands_flush(world)
```

## Resources (Singletons)

```nx
// Global resources (one per type)
struct DeltaTime { dt: f32 }
struct GameState { score: i32, wave: i32 }

ecs_insert_resource(world, DeltaTime { dt })
ecs_insert_resource(world, GameState { score: 0, wave: 1 })

// Access in systems
fn system(world: *World) {
    let dt = ecs_get_resource(world, DeltaTime).dt
    let mut state = ecs_get_resource_mut(world, GameState)
    state.score += 10
}
```

## Complete Example

```nx
import runtime.ecs
import runtime.gfx
import runtime.input

struct Pos { x: f32, y: f32 }
struct Vel { x: f32, y: f32 }
struct Player { speed: f32 }
struct Bullet { }
struct Enemy { hp: i32 }

fn main() {
    let world = ecs_world_create()
    defer ecs_world_destroy(world)
    
    ecs_register_component(world, Pos)
    ecs_register_component(world, Vel)
    ecs_register_component(world, Player)
    ecs_register_component(world, Bullet)
    ecs_register_component(world, Enemy)
    
    // Player
    let player = ecs_entity_create(world)
    ecs_add(world, player, Pos { x: 400, y: 300 })
    ecs_add(world, player, Vel { x: 0, y: 0 })
    ecs_add(world, player, Player { speed: 300 })
    
    gfx_init(800, 600, "ECS Demo")
    defer gfx_quit()
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()
        input_poll_events()
        
        // Player input system
        for entity in ecs_query(world, [Pos, Vel, Player]) {
            let pos = ecs_get_mut(world, entity, Pos)
            let vel = ecs_get_mut(world, entity, Vel)
            let p = ecs_get(world, entity, Player)
            
            vel.x = 0; vel.y = 0
            if input_key_down(KEY_LEFT)  { vel.x = -p.speed }
            if input_key_down(KEY_RIGHT) { vel.x =  p.speed }
            if input_key_down(KEY_UP)    { vel.y = -p.speed }
            if input_key_down(KEY_DOWN)  { vel.y =  p.speed }
        }
        
        // Movement system
        for entity in ecs_query(world, [Pos, Vel]) {
            let pos = ecs_get_mut(world, entity, Pos)
            let vel = ecs_get(world, entity, Vel)
            pos.x += vel.x * dt
            pos.y += vel.y * dt
        }
        
        // Render
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        for entity in ecs_query(world, [Pos]) {
            let pos = ecs_get(world, entity, Pos)
            gfx_draw_rect(pos.x, pos.y, 20, 20, 0xffffff)
        }
        gfx_end_frame()
    }
}
```