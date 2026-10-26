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

## NEXUS Poker

[`examples/poker_gui.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/poker_gui.nx) is a windowed Texas Hold'em game against an AI dealer built on `std.engine2d`, `std.sprite2d` and `std.audio2d`.

- 52 generated card textures plus a card back under `examples/assets/cards/`.
- Staggered deal animation: each card slides from the deck to its seat.
- Bet-size buttons on keys `1`-`5` (50 / 100 / 200 / 500 / all-in, plus a 25 ante).
- Dealer AI evaluates its made seven-card hand and folds, calls or raises; a raise the player cannot match forces a fold.
- Distinct WAV effects for deal, chips, win, lose and fold under `examples/assets/sounds/`.
- Chip stack persists between sessions in `poker_save.txt`; `R` resets it to 1000.

```bash
cd examples
./run_poker.bat
```

`run_poker.bat` switches to the `examples/` directory, puts the installed SDL runtime on `PATH`, rebuilds the example with the LLVM backend and launches it, so it can be started from any directory. Running `poker_gui.exe` by hand still needs `NEXUS\runtime` on `PATH` (or the SDL DLLs next to the executable) and a working directory where `assets/` or `examples/assets/` resolves.

## Other application demos

- `examples/nexus_focus.nx`: graphical focus/break timer with mouse buttons, sound tone and a small session-count file.
- `examples/nexus_editor.nx`: experimental 2D scene/box editor built on the current ECS, UI and snapshot helpers.
- `examples/nexus_studio.nx`: terminal-menu demo with calculator, guessing-game and mini-battle routines.
- `examples/nexus_paint.nx`: native raster painting tool; documented under [Paint Template](../templates/paint.md).

These examples are meant to show what the compiler/runtime currently supports. Check source and backend requirements before relying on a capability in your own application.
