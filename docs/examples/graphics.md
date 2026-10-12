# Graphics Examples

Graphics and rendering demos.

## Native App

`examples/native_app.nx` — Minimal application template.

```nx
gfx_init(800, 600, "App")
while !gfx_should_close() {
    gfx_begin_frame()
    gfx_clear(0x1a1a2e)
    gfx_end_frame()
}
```

## Native Graphics Game

`examples/native_graphics_game.nx` — 2D game with sprites, input, collision.

**Features:**
- Player sprite
- Enemy AI
- Collision detection
- Score display
- Background rendering

## Native 3D

`examples/native_3d.nx` — 3D rendering with OpenGL.

**Features:**
- Rotating cube
- Camera controls (WASD + mouse)
- Basic lighting
- Texture mapping
- Wireframe toggle

## Model Demo

`examples/model_demo.nx` — 3D model loading.

**Features:**
- OBJ loader
- Multiple meshes
- Material support
- Animation (future)

## Particles Demo

`examples/particles_demo.nx` — Particle system showcase.

**Effects:**
- Fire
- Smoke
- Explosion
- Magic sparkles
- Rain
- Snow

## V080 Runtime Showcase

`examples/v080_runtime_showcase.nx` — Legacy graphics demo.

**Features:**
- Sprite rendering
- Text rendering
- Primitive shapes
- Color blending

## Ultimate Showcase

`examples/ultimate_showcase.nx` — All graphics features.

**Features:**
- 2D sprites
- 3D meshes
- Particles
- Post-processing (future)
- UI overlay