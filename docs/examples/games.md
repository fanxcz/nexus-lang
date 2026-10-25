# Game Examples

The repository includes several games/prototypes. They range from terminal simulations to small native graphical demonstrations; none should be interpreted as a production or multiplayer engine unless the source actually implements that scope.

## Neon Survivor

`examples/neon_survivor.nx` is the game template source. It has a start screen, arena movement, mouse aiming/shooting, basic enemies, projectile slots, score/wave/health state, pause and game-over flow.

```bash
nexus check examples/neon_survivor.nx
nexus run examples/neon_survivor.nx
```

## Terminal Arena

`examples/nexus_arena.nx` is a text-mode turn-based hero-versus-boss simulation with HP/mana/levels, actions, rounds and score/rank values. It is a terminal program, not a real-time multiplayer arena shooter.

```bash
nexus run examples/nexus_arena.nx
```

## Terminal movement game

`examples/terminal_game.nx` draws a simple `@` player in a terminal grid, accepts WASD movement and Q to quit, and updates a counter. It depends on terminal input behavior, which can vary between shells/hosts.

## Small graphical game

`examples/native_graphics_game.nx` demonstrates 2D movement, a mouse-controlled target, score display and basic primitive drawing. See [Graphics Examples](graphics.md).

## Tilemap platformer

[`examples/game2d_platformer.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/game2d_platformer.nx) is a no-external-assets platformer demo using `std.tilemap`: grid-based platforms, horizontal/vertical collision resolution, gravity, jumping and a simple colored tile renderer. The tilemap API is documented under [2D Game Helpers and Tilemaps](../runtime/game2d.md).

## Other application demos

- `examples/nexus_focus.nx`: graphical focus/break timer with mouse buttons, sound tone and a small session-count file.
- `examples/nexus_editor.nx`: experimental 2D scene/box editor built on the current ECS, UI and snapshot helpers.
- `examples/nexus_studio.nx`: terminal-menu demo with calculator, guessing-game and mini-battle routines.
- `examples/nexus_paint.nx`: native raster painting tool; documented under [Paint Template](../templates/paint.md).

These examples are meant to show what the compiler/runtime currently supports. Check source and backend requirements before relying on a capability in your own application.
