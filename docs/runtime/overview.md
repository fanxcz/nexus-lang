# Runtime Overview

The NEXUS runtime provides a comprehensive set of APIs for building games and applications, including graphics, audio, input, ECS, physics, and UI.

## Module Index

| Module | Description | Status |
|--------|-------------|--------|
| [`Memory`](memory.md) | Allocation, arenas, allocators | ✅ Stable |
| [`Graphics 2D`](graphics-2d.md) | SDL2 renderer, sprites, text | ✅ Stable |
| [`Graphics 3D`](graphics-3d.md) | OpenGL, meshes, shaders | ✅ Stable |
| [`Audio`](audio.md) | Sound effects, music, mixing | ✅ Stable |
| [`Input`](input.md) | Keyboard, mouse, gamepad | ✅ Stable |
| [`ECS`](ecs.md) | Entity Component System | ✅ Stable |
| [`Scenes`](scenes.md) | Scene management, transitions | ✅ Stable |
| [`Physics`](physics.md) | 2D physics (Box2D-style) | ✅ Stable |
| [`Particles`](particles.md) | Particle systems | ✅ Stable |
| [`UI`](ui.md) | Immediate-mode UI | ✅ Stable |

## Initialization

Most runtime modules require initialization:

```nx
import runtime.gfx
import runtime.audio
import runtime.input

fn main() {
    // Initialize graphics (required first)
    gfx_init(800, 600, "My Game")
    defer gfx_quit()
    
    // Initialize audio
    audio_init()
    defer audio_quit()
    
    // Initialize input
    input_init()
    defer input_quit()
    
    // Main loop
    while !gfx_should_close() {
        gfx_begin_frame()
        input_poll_events()
        
        // Your game logic here
        
        gfx_end_frame()
    }
}
```

## Graphics Backend (v1.1.0)

NEXUS 1.1.0 uses **SDL2 2D renderer** by default for windowed applications. OpenGL is activated lazily when 3D functions are called.

```nx
import runtime.gfx

fn main() {
    gfx_init(800, 600, "Backend Demo")
    defer gfx_quit()
    
    // Check active backend
    println("Backend: {}", gfx_backend())  // "sdl2" or "opengl"
    
    // Window controls
    gfx_fullscreen(true)   // Fullscreen
    gfx_fullscreen(false)  // Windowed
    gfx_maximize()         // Maximize
    gfx_restore()          // Restore
    
    // Virtual UI resolution (stable coordinates)
    gfx_set_ui_resolution(1920, 1080)
    
    while !gfx_should_close() {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        gfx_end_frame()
    }
}
```

## Coordinate Systems

### Screen Coordinates
- Origin: Top-left (0, 0)
- X: Right, Y: Down
- Pixels

### Virtual UI Coordinates
- Set with `gfx_set_ui_resolution(width, height)`
- Origin: Top-left (0, 0)
- Stable across window resize/fullscreen
- Mouse coordinates automatically converted

### World Coordinates (3D)
- Right-handed: X right, Y up, Z forward
- Origin: Center (0, 0, 0)
- Units: Meters (configurable)

## Resource Management

Resources (textures, sounds, fonts) are loaded and managed:

```nx
import runtime.gfx
import runtime.audio

fn main() {
    gfx_init(800, 600, "Resources")
    defer gfx_quit()
    
    audio_init()
    defer audio_quit()
    
    // Load once, use many times
    let texture = gfx_load_texture("assets/player.png")
    let sound = audio_load_sound("assets/jump.wav")
    let font = gfx_load_font("assets/font.ttf", 24)
    
    // Resources automatically freed on gfx_quit()/audio_quit()
    // Or manually:
    // gfx_free_texture(texture)
    // audio_free_sound(sound)
    // gfx_free_font(font)
    
    while !gfx_should_close() {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        gfx_draw_texture(texture, 100, 100)
        gfx_draw_text(100, 200, "Hello!", 0xffffff, font)
        
        if input_key_pressed(KEY_SPACE) {
            audio_play_sound(sound)
        }
        
        gfx_end_frame()
    }
}
```

## Delta Time

```nx
import runtime.gfx

fn main() {
    gfx_init(800, 600, "Delta Time")
    defer gfx_quit()
    
    let mut player_x = 400.0
    let speed = 300.0  // pixels per second
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()  // Seconds since last frame
        
        if input_key_down(KEY_RIGHT) {
            player_x += speed * dt
        }
        
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        gfx_draw_rect(player_x, 300, 50, 50, 0xffffff)
        gfx_end_frame()
    }
}
```

## Common Patterns

### Game Loop Structure

```nx
fn main() {
    init()
    defer cleanup()
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()
        
        // 1. Input
        input_poll_events()
        handle_input(dt)
        
        // 2. Update
        update(dt)
        
        // 3. Render
        gfx_begin_frame()
        gfx_clear(bg_color)
        render()
        gfx_end_frame()
    }
}
```

### Fixed Timestep (for physics)

```nx
const FIXED_DT = 1.0 / 60.0
let mut accumulator = 0.0

while !gfx_should_close() {
    let frame_dt = gfx_delta_time()
    accumulator += frame_dt
    
    while accumulator >= FIXED_DT {
        fixed_update(FIXED_DT)
        accumulator -= FIXED_DT
    }
    
    let alpha = accumulator / FIXED_DT
    render(alpha)  // Interpolate for smooth rendering
}
```

## Error Handling

Runtime functions return `Result` or use `bool` for success:

```nx
// Texture loading
let texture = gfx_load_texture("missing.png")?
// Returns Result<Texture, GfxError>

// Audio
let stream = audio_load_music("music.ogg")?
// Returns Result<Music, AudioError>

// Font
let font = gfx_load_font("font.ttf", 16)?
// Returns Result<Font, GfxError>
```

## Threading

NEXUS runtime is **not thread-safe**. All runtime calls must be made from the main thread. For background tasks, use `std.thread` (planned) with message passing to the main thread.

## Version Compatibility

| Runtime Version | NEXUS Version | Notes |
|-----------------|---------------|-------|
| 1.1.x | 1.1.x | Current stable |
| 1.0.x | 1.0.x | Legacy (OpenGL-only 2D) |

See [CHANGELOG](https://github.com/fanxcz/nexus/blob/main/CHANGELOG.md) for detailed changes.