# Templates

Project templates for `nexus new`.

## Available Templates

| Template | Command | Description |
|----------|---------|-------------|
| `app` | `nexus new --template app` | Basic application |
| `game` | `nexus new --template game` | 2D game starter |
| `paint` | `nexus new --template paint` | Nexus Paint |
| `cli` | `nexus new --template cli` | CLI application |
| `library` | `nexus new --template library` | Library package |
| `editor` | `nexus new --template editor` | Code editor |
| `server` | `nexus new --template server` | HTTP server |
| `3d` | `nexus new --template 3d` | 3D graphics |

## Template Details

### app

```bash
nexus new MyApp --template app
```

Basic application with main loop:
```nx
import std.io
import runtime.gfx

fn main() {
    gfx_init(800, 600, "My App")
    defer gfx_quit()
    
    while !gfx_should_close() {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        gfx_draw_text(10, 10, "Hello!", 0xffffff)
        gfx_end_frame()
    }
}
```

### game

```bash
nexus new MyGame --template game
```

2D game with player, input, rendering:
```nx
import runtime.gfx
import runtime.input

struct Player { x: f32, y: f32, speed: f32 }

fn main() {
    gfx_init(800, 600, "My Game")
    defer gfx_quit()
    
    let mut player = Player { x: 400, y: 300, speed: 300 }
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()
        input_poll_events()
        
        if input_key_down(KEY_LEFT)  { player.x -= player.speed * dt }
        if input_key_down(KEY_RIGHT) { player.x += player.speed * dt }
        if input_key_down(KEY_UP)    { player.y -= player.speed * dt }
        if input_key_down(KEY_DOWN)  { player.y += player.speed * dt }
        
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        gfx_draw_rect(player.x, player.y, 32, 32, 0xffffff)
        gfx_end_frame()
    }
}
```

### paint

```bash
nexus new MyPaint --template paint
```

Full raster editor (Nexus Paint):
- 1024x720 canvas
- Pencil, eraser, line, rectangle, fill
- Color palette, brush size
- Undo/redo
- BMP save/load

### cli

```bash
nexus new MyCLI --template cli
```

Command-line app with argument parsing:
```nx
import std.io
import std.os

fn main() {
    let args = os.args()
    if args.len < 2 {
        println("Usage: mycli <command>")
        return
    }
    
    match args[1] {
        "hello" => println("Hello, {}!", args.get(2).unwrap_or("World")),
        "version" => println("1.0.0"),
        _ => println("Unknown command: {}", args[1]),
    }
}
```

### library

```bash
nexus new MyLib --template library
```

Library package structure:
```
MyLib/
├── nexus.toml
├── src/
│   ├── lib.nx      # Main export
│   └── math.nx     # Submodule
├── tests/
└── examples/
```

### editor

```bash
nexus new MyEditor --template editor
```

Code editor with syntax highlighting:
- Line numbers
- Syntax highlighting for NEXUS
- Find/replace
- Multiple tabs

### server

```bash
nexus new MyServer --template server
```

HTTP server template:
```nx
import std.net

fn main() {
    // Simple GET
    let resp = net.get("https://api.example.com/data")?
    println("Response: {}", resp.text()?)
    
    // Server (future)
    // net.listen("127.0.0.1:8080", |req| {
    //     net.respond(200, "OK", "Hello!")
    // })
}
```

### 3d

```bash
nexus new My3D --template 3d
```

3D graphics with OpenGL:
```nx
import runtime.gfx3d

fn main() {
    gfx_init(800, 600, "3D Demo")
    defer gfx_quit()
    
    let shader = gfx3d_create_shader(vertex_src, fragment_src)
    let mesh = gfx3d_create_cube()
    
    while !gfx_should_close() {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        gfx3d_use_shader(shader)
        gfx3d_set_uniform_mat4(shader, "uProj", proj)
        gfx3d_set_uniform_mat4(shader, "uView", view)
        gfx3d_set_uniform_mat4(shader, "uModel", model)
        gfx3d_draw_mesh(mesh)
        
        gfx_end_frame()
    }
}
```

## Repository Template Sources

The built-in starter sources are flat `.nx` files under `examples/templates/` in the NEXUS source repository. Create a project with `nexus new MyProject --template app`; the command copies the selected starter into the project's `src/main.nx` and writes `nexus.toml`.

A general user-defined directory-template registry is not part of the current stable CLI.

## Available Built-in Templates

```bash
nexus new --list
# app         Basic application
# game        2D game starter
# paint       Nexus Paint
# cli         CLI application
# library     Library package
# editor      Code editor
# server      HTTP server
# 3d          3D graphics
```