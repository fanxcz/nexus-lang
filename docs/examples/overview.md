# Examples Overview

Browse and run examples from the repository.

## Running Examples

```bash
# From repository root
nexus run examples/hello.nx

# Or create from template
nexus new MyGame --template game
```

## Example Categories

### Basics

| Example | Description | Concepts |
|---------|-------------|----------|
| `hello.nx` | Hello World | Basic I/O |
| `imports.nx` | Module imports | `import` syntax |
| `control_flow.nx` | If/else, loops | Control flow |
| `else_if.nx` | Else-if chains | Branching |
| `break_continue.nx` | Loop control | `break`, `continue` |
| `arrays.nx` | Array basics | Fixed-size arrays |
| `structs.nx` | Struct definitions | `struct`, `impl` |
| `enums_match.nx` | Enums & pattern matching | `enum`, `match` |
| `pointers.nx` | Raw pointers | `*T`, `mem_alloc` |
| `memory.nx` | Memory management | `mem_alloc`, `mem_free` |
| `factorial.nx` | Recursion | Recursive functions |
| `float.nx` | Floating point | `f32`, `f64` |

### Desktop Applications

| Example | Description | Concepts |
|---------|-------------|----------|
| `nexus_focus.nx` | Native Pomodoro-style focus timer with start/pause/reset, focus and break modes, progress bar, session counter, and saved session summary | SDL2 graphics, mouse input, files, time |

Run it from the repository root:

```bash
nexus check examples/nexus_focus.nx
nexus run examples/nexus_focus.nx
```

The session count is saved to `nexus_focus_sessions.txt` in the working directory. The current source example is a single standalone `.nx` file and does not require creating a project.

### Standard Library

| Example | Description | Modules |
|---------|-------------|---------|
| `interactive_cli.nx` | Interactive CLI | `std.io`, `std.fs` |
| `utility.nx` | Utility functions | Various std |

### Graphics & Games

| Example | Description | Runtime APIs |
|---------|-------------|--------------|
| `native_app.nx` | Basic app template | `gfx_init`, loop |
| `native_graphics_game.nx` | 2D game demo | `gfx_draw_*`, input |
| `native_3d.nx` | 3D graphics | `gfx3d_*`, OpenGL |
| `model_demo.nx` | 3D model loading | `gfx3d_load_model` |
| `particles_demo.nx` | Particle system | `particles_*` |
| `v080_game_framework.nx` | Game framework | ECS, scenes |
| `v080_runtime_showcase.nx` | Runtime showcase | All runtime APIs |

### Advanced Games

| Example | Description | Features |
|---------|-------------|----------|
| `neon_survivor.nx` | Twin-stick survivor | ECS, particles, waves |
| `nexus_arena.nx` | Arena shooter | Multiplayer-ready |
| `nexus_paint.nx` | Paint application | Canvas, tools, undo |
| `nexus_editor.nx` | Code editor | Syntax highlight, LSP |
| `nexus_studio.nx` | Game studio/IDE | Project mgmt, scenes |
| `nexus_v1_full_stack.nx` | Full stack demo | HTTP, DB, graphics |
| `ultimate_showcase.nx` | Feature showcase | All runtime features |
| `terminal_game.nx` | Terminal-based game | Text UI, input |

### Audio

| Example | Description | APIs |
|---------|-------------|------|
| `audio_demo.nx` | Audio playback | `audio_play_sound`, music |

### FFI

| Example | Description | APIs |
|---------|-------------|------|
| `ffi.nx` | C FFI | `extern "C"` |

## Template Examples

Starter sources live in `examples/templates/` in the repository. Create a project from one with `nexus new MyGame --template game`; the command writes a normal project with `src/main.nx` and `nexus.toml`.

```bash
nexus new MyGame --template game
cd MyGame
nexus build
nexus run
```

| Template | File | Description |
|----------|------|-------------|
| `app` | `app.nx` | Basic application |
| `game` | `game.nx` | 2D game starter |
| `paint` | `paint.nx` | Nexus Paint |
| `cli` | `cli.nx` | CLI application |
| `library` | `library.nx` | Library package |
| `editor` | `editor.nx` | Code editor |
| `server` | `server.nx` | HTTP server |
| `3d` | `3d.nx` | 3D graphics |

## Contributing Examples

1. Add `.nx` file to `examples/`
2. Follow naming: `snake_case.nx`
3. Include comment header:
   ```nx
   // Example: Brief description
   // Demonstrates: feature1, feature2
   // Run: nexus run --example example_name
   ```
4. Keep examples focused and minimal
4. Update this page