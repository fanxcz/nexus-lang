# Examples Overview

The `examples/` directory contains programs for compiler/language checks, simple applications, games, graphics, audio and runtime helpers. Examples are standalone `.nx` files unless stated otherwise.

## Run an example

From the repository root:

```bash
nexus check examples/hello.nx
nexus run examples/hello.nx
```

The CLI accepts a source-file path directly; `nexus run --example NAME` is not a supported command. If a source relies on relative assets, keep the repository root as the working directory when running it.

## Language basics

| Source | Demonstrates |
|---|---|
| `hello.nx` | Basic function and output |
| `control_flow.nx`, `else_if.nx`, `break_continue.nx` | Conditional branches and loops |
| `arrays.nx`, `structs.nx`, `enums_match.nx` | Data types and pattern matching |
| `imports.nx` | Project-relative module imports |
| `factorial.nx` | Recursion |
| `float.nx` | Floating-point values |
| `memory.nx`, `pointers.nx` | Low-level memory/pointer examples |
| `interactive_cli.nx` | Prompted terminal input |

## Applications and games

| Source | What it actually contains |
|---|---|
| `nexus_focus.nx` | A graphical focus timer using mouse controls, sound tone and a small text session file |
| `nexus_paint.nx` | Raster drawing canvas with brush/eraser/line/rectangle/fill tools, undo/redo and BMP save/load |
| `nexus_editor.nx` | Experimental 2D scene/box editor built from drawing, UI helper, ECS and scene snapshot APIs |
| `neon_survivor.nx` | Arena survivor prototype with player movement, mouse shooting and enemy waves/state |
| `nexus_arena.nx` | Terminal turn-based hero-versus-boss combat simulation |
| `terminal_game.nx` | Minimal terminal-grid movement game |
| `cheat_engine.nx` | Windows-specific memory scanner/writer using FFI and `ReadProcessMemory`/`WriteProcessMemory`; only use against processes you own or are authorized to inspect, with administrator rights as required |
| `nexus_studio.nx` | Terminal menu demo containing calculator, guessing and mini-battle routines |
| `task_manager.nx` | Windows-only console sample using `tasklist`, `taskkill`, `systeminfo` and `start`; names/PIDs are restricted before shell commands |

The names describe individual sample programs, not claims that each is a complete production product or game framework. The memory scanner is a low-level FFI demonstration, not a protected or sandboxed process-management tool; use it only on software and systems you are authorized to inspect or modify. The Task Manager sample invokes Windows shell commands through the runtime and is not a cross-platform process-management API.

## Graphics and runtime

| Source | Demonstrates |
|---|---|
| `native_app.nx` | Window, text, rectangle and mouse polling |
| `native_graphics_game.nx` | Simple 2D movement, mouse state and primitive drawing |
| `game2d_platformer.nx` | Tilemap platforms, axis-separated collision resolution, gravity and jumping |
| `native_3d.nx` | 3D grid/cubes and camera setup |
| `model_demo.nx` | Load and draw the repository's OBJ cube asset |
| `particles_demo.nx` | Create, update, draw and clear simple runtime particles |
| `audio_demo.nx` | Load and play a WAV file through the optional runtime audio backend |
| `ultimate_showcase.nx` | Small integrated runtime showcase; inspect source for exact scope |

## Source templates

Templates live in `examples/templates/`. The `nexus new` command copies one selected template to a generated project's `src/main.nx`; it does not create a multi-module framework. See [Project Templates](../templates/index.md).
