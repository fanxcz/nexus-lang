# Quick Start

Create your first NEXUS project in under a minute.

## 1. Create a Project

```bash
# Interactive project creation
nexus new MyGame --template game

# Or specify all options
nexus new MyApp --template app --name "My Application" --author "Your Name"
```

## 2. Project Structure

```
MyGame/
├── nexus.toml          # Project configuration
├── src/
│   └── main.nx         # Entry point
├── assets/             # Game assets (images, sounds, fonts)
├── vendor/             # Local packages (gitignored)
├── nexus.lock          # Package lockfile (gitignored)
└── build/              # Build output (gitignored)
```

## 3. Project Configuration (`nexus.toml`)

```toml
[package]
name = "my-game"
version = "1.0.0"
edition = "2026"
description = "A cool NEXUS game"
authors = ["Your Name <you@example.com>"]
license = "MIT"
repository = "https://github.com/you/my-game"

[build]
entry = "src/main.nx"
optimization = "speed"      # "speed" | "size" | "debug"
debug_symbols = true
target_cpu = "native"       # or "x86-64-v3", "arm64", etc.

[targets]
default = "x86_64-linux"
# Cross-compilation targets:
# windows = "x86_64-windows"
# macos = "x86_64-macos"
# linux_arm = "aarch64-linux"

[dependencies]
# Local packages (vendored in vendor/)
# my-lib = { path = "vendor/my-lib" }

[dev-dependencies]
# Test-only dependencies
```

## 4. Write Code (`src/main.nx`)

```nx
import std.io
import std.memory
import runtime.gfx
import runtime.input

fn main() {
    // Initialize graphics
    gfx_init(800, 600, "My First NEXUS Game")
    defer gfx_quit()

    // Game state
    let player_x = 400.0
    let player_y = 300.0
    let speed = 300.0  // pixels per second

    // Main loop
    while !gfx_should_close() {
        let dt = gfx_delta_time()

        // Input handling
        if input_key_down(KEY_LEFT)  { player_x -= speed * dt }
        if input_key_down(KEY_RIGHT) { player_x += speed * dt }
        if input_key_down(KEY_UP)    { player_y -= speed * dt }
        if input_key_down(KEY_DOWN)  { player_y += speed * dt }

        // Render
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        // Draw player (white square)
        gfx_draw_rect(player_x - 16, player_y - 16, 32, 32, 0xffffff)
        
        gfx_draw_text(10, 10, "Arrow keys to move", 0x888888)
        gfx_end_frame()
    }
}
```

## 5. Build and Run

```bash
cd MyGame

# Debug build (default)
nexus build
nexus run

# Release build
nexus build --release
nexus run --release
```

## 6. Common Commands

| Command | Description |
|---------|-------------|
| `nexus new <name> --template <t>` | Create new project |
| `nexus init` | Initialize project in current directory |
| `nexus build` | Build project (debug) |
| `nexus build --release` | Build optimized release |
| `nexus run` | Build and run |
| `nexus check` | Type-check without building |
| `nexus clean` | Clean build artifacts |
| `nexus fmt` | Format source code |
| `nexus lint` | Lint source code |
| `nexus doc` | Generate documentation |
| `nexus test` | Run tests |
| `nexus pkg add <path>` | Add local package |
| `nexus pkg build` | Build all packages |
| `nexus editor` | Open built-in code editor |
| `nexus lsp` | Start Language Server |

## Next Steps

- Read the [Language Reference](../language/overview.md)
- Explore [Standard Library](../std/overview.md)
- Learn [Runtime API](../runtime/overview.md)
- Check out [Examples](../examples/overview.md)
- Try the [Paint Template](../templates/paint.md)