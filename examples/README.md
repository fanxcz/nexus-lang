# Examples

This directory contains small programs used to demonstrate NEXUS language features, runtime APIs, games, and desktop tools. The examples are kept as source files so they can be checked and run with the compiler.

For the broader feature guides, start at [the examples documentation](../docs/examples/index.md).

## Start here

- [`hello.nx`](hello.nx) — smallest runnable program.
- [`new_features_demo.nx`](new_features_demo.nx) — compact feature showcase.
- [`native_graphics_game.nx`](native_graphics_game.nx) — simple native 2D game loop.
- [`game2d_platformer.nx`](game2d_platformer.nx) — tilemap-based platformer.
- [`poker_gui.nx`](poker_gui.nx) — NEXUS Poker 2.1.0, mouse-driven Texas Hold'em with a stable custom cursor, persistent cosmetic table-skin shop, and shared art/audio in [`assets/`](assets/README.md).

## Language basics

Small, focused source files cover arrays, loops, branching, functions, structs, enums, pointers, memory, floats, imports, and FFI. Representative files include `arrays.nx`, `break_continue.nx`, `control_flow.nx`, `else_if.nx`, `enums_match.nx`, `factorial.nx`, `ffi.nx`, `float.nx`, `imports.nx`, `lists_demo.nx`, `memory.nx`, `pointers.nx`, `structs.nx`, and `utility.nx`.

## Games and graphical runtime

- [`poker_gui.nx`](poker_gui.nx) — graphical No-Limit Texas Hold'em.
- [`poker.nx`](poker.nx) — terminal poker example.
- [`neon_survivor.nx`](neon_survivor.nx) — arena survival game.
- [`game2d_platformer.nx`](game2d_platformer.nx) — 2D tilemap platformer.
- [`nexus_arena.nx`](nexus_arena.nx) and [`terminal_game.nx`](terminal_game.nx) — terminal game examples.
- [`audio_demo.nx`](audio_demo.nx), [`fonts_demo.nx`](fonts_demo.nx), [`model_demo.nx`](model_demo.nx), [`native_3d.nx`](native_3d.nx), and [`particles_demo.nx`](particles_demo.nx) — focused runtime demonstrations.
- `v080_game_framework.nx` and `v080_runtime_showcase.nx` — explicitly version-labelled legacy examples.

## Interactive applications

`nexus_paint.nx`, `nexus_editor.nx`, `nexus_focus.nx`, `nexus_studio.nx`, `task_manager.nx`, and `interactive_cli.nx` demonstrate larger interactive applications and workflows.

## Templates

Starter files live in [`templates/app.nx`](templates/app.nx), [`templates/game.nx`](templates/game.nx), and the other files in `templates/`. Some are intentionally similar to complete demos: keep these copies when updating project scaffolding, and do not treat them as accidental duplicate files.

## Check and run

From the repository root:

```sh
nexus check examples/hello.nx
nexus run examples/hello.nx
nexus check examples/poker_gui.nx
nexus check examples/vector_lab.nx
nexus run examples/vector_lab.nx
```

On Windows, `run_poker.bat` builds and launches the Poker GUI using the installed NEXUS toolchain. The script expects NEXUS under `%LOCALAPPDATA%\\Programs\\NEXUS`; edit the script if the installation is elsewhere.

Some examples load assets through repository-relative paths. Run them from the repository root unless their own launcher changes the working directory. See [`assets/README.md`](assets/README.md) before moving or removing files under `assets/`.
