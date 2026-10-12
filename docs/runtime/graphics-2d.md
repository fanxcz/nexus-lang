# Graphics 2D (SDL2 Renderer)

NEXUS 2D graphics use the **SDL2 hardware-accelerated renderer** by default. No OpenGL context is created unless 3D functions are called.

## Initialization

```nx
import runtime.gfx

fn main() {
    // width, height, title
    let ok = gfx_init(800, 600, "My Game")
    if !ok {
        print("Failed to init: " + gfx_error())
        return
    }
    defer gfx_quit()

    // Optional: VSync
    gfx_vsync(true)

    while !gfx_should_close() {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        // ... draw calls ...
        gfx_end_frame()
    }
}
```

## Drawing Primitives

```nx
// Rectangles
gfx_draw_rect(x, y, w, h, color)           // Filled
gfx_draw_rect_outline(x, y, w, h, color)   // Outline

// Circles
gfx_draw_circle(x, y, radius, color)
gfx_draw_circle_outline(x, y, radius, color)

// Lines
gfx_draw_line(x1, y1, x2, y2, color, thickness)

// Points
gfx_draw_point(x, y, color)
```

### Color Format
Colors are **32-bit ARGB hex**: `0xAARRGGBB`
- `0xffffffff` = white
- `0xff000000` = black
- `0xffff0000` = red
- `0xff00ff00` = green
- `0xff0000ff` = blue
- `0x80ffffff` = 50% transparent white

## Text Rendering

```nx
// Default font (built-in 8x8)
gfx_draw_text(x, y, "Hello", 0xffffffff)

// With custom font
let font = gfx_load_font("assets/font.ttf", 24)
gfx_draw_text(x, y, "Hello", 0xffffffff, font)
gfx_free_font(font)
```

## Textures/Sprites

```nx
// Load from file (PNG, BMP, JPG)
let tex = gfx_load_texture("assets/sprite.png")

// Draw at position
gfx_draw_texture(tex, x, y)

// Draw with source rect (sprite sheet)
gfx_draw_texture_ex(tex, dst_x, dst_y, src_x, src_y, src_w, src_h)

// Draw with scale/rotation
gfx_draw_texture_ex(tex, x, y, 0, 0, 0, 0, scale_x, scale_y, rotation, center_x, center_y)

// Free when done (auto-freed on gfx_quit)
gfx_free_texture(tex)
```

## Canvas (Render Targets)

```nx
// Create off-screen canvas
let canvas = gfx_create_canvas(512, 512)

// Set as render target
gfx_set_canvas(canvas)
gfx_clear(0x00000000)
gfx_draw_circle(256, 256, 100, 0xffff0000)
gfx_set_canvas(null)  // Back to screen

// Use canvas as texture
gfx_draw_texture(canvas, 100, 100)
```

## Window Controls

```nx
// Fullscreen
gfx_fullscreen(true)   // Borderless fullscreen
gfx_fullscreen(false)  // Windowed

// Maximize / Restore
gfx_maximize()
gfx_restore()

// Get window size
let w = gfx_width()
let h = gfx_height()

// Virtual UI resolution (mouse coords normalized)
gfx_set_ui_resolution(1920, 1080)
// Mouse now reports 0..1920, 0..1080 regardless of window size

// Check active backend
print(gfx_backend())  // "sdl2" or "opengl"
```

## Frame Timing

```nx
while !gfx_should_close() {
    let dt = gfx_delta_time()  // Seconds since last frame
    let fps = gfx_fps()        // Current FPS
    
    // Fixed timestep for physics
    // ...
}
```

## Blend Modes

```nx
gfx_set_blend_mode(BLEND_ALPHA)      // Default: alpha blend
gfx_set_blend_mode(BLEND_ADD)        // Additive (particles, glow)
gfx_set_blend_mode(BLEND_MOD)        // Multiply
gfx_set_blend_mode(BLEND_NONE)       // No blending (opaque)
```

## Complete Example

```nx
import runtime.gfx
import runtime.input

fn main() {
    gfx_init(800, 600, "2D Demo")
    defer gfx_quit()
    gfx_vsync(true)

    let tex = gfx_load_texture("assets/player.png")
    let font = gfx_load_font("assets/font.ttf", 16)
    
    let mut x = 400.0
    let mut y = 300.0
    let speed = 300.0

    while !gfx_should_close() {
        let dt = gfx_delta_time()
        input_poll_events()

        if input_key_down(KEY_LEFT)  { x -= speed * dt }
        if input_key_down(KEY_RIGHT) { x += speed * dt }
        if input_key_down(KEY_UP)    { y -= speed * dt }
        if input_key_down(KEY_DOWN)  { y += speed * dt }

        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        gfx_draw_texture(tex, x - 32, y - 32)
        gfx_draw_text(10, 10, "WASD to move", 0x888888, font)
        gfx_draw_text(10, 30, "FPS: " + str_f64(gfx_fps()), 0x888888, font)
        
        gfx_end_frame()
    }
}
```

## Backend Notes

| Backend | When Used | Features |
|---------|-----------|----------|
| `sdl2` | Default for 2D | Fast, no GL context, works everywhere |
| `opengl` | When 3D called | Full GL access, shaders, 3D |

Call `gfx_backend()` to check active renderer.