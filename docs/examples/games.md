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

## Vector Lab

[`examples/vector_lab.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/vector_lab.nx) is a small playable arena sample using the `std.vec2`, `std.collision2d` and `std.animation2d` modules. Move with WASD or arrow keys, slide along obstacles, and collect the pulsing beacon. It demonstrates delta-time movement, vector normalization, circle-vs-rectangle collision and a ping-pong animation curve.

```bash
nexus check examples/vector_lab.nx
nexus run examples/vector_lab.nx
```

## Tilemap platformer

[`examples/game2d_platformer.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/game2d_platformer.nx) is a no-external-assets platformer demo using `std.tilemap`: grid-based platforms, horizontal/vertical collision resolution, gravity, jumping and a simple colored tile renderer. The tilemap API is documented under [2D Game Helpers and Tilemaps](../runtime/game2d.md).

## NEXUS Poker

The game uses a minimal graphite-and-yellow identity, a single N mark in the upper-left, flat seat monograms, and a graphite-and-gold card back. A dedicated Windows packaging workflow creates a ZIP for distribution. See the [standalone instructions and release guide](https://github.com/fanxcz/nexus-lang/blob/main/examples/POKER_README.md).

[`examples/poker_gui.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/poker_gui.nx) is a windowed no-limit Texas Hold'em table for the mouse, built on `std.engine2d`, `std.sprite2d` and `std.audio2d`. The 2.1.0 update corrects five-card hand scoring, compares kickers in order, and requires all five cards to share a suit for a straight flush. The menu also includes a persistent cosmetic shop with blue, purple and crimson table themes bought with chips earned in play. The custom cursor keeps a fixed size to avoid hover-edge pulsing.

- Main menu with a 1-5 bot selector. Every seat starts with 1,000 chips and blinds are 10 / 20.
- Complete Hold'em betting: pre-flop, flop, turn and river; check, call, fold, raise with a draggable slider, and all-in. Side pots, split pots and seven-card hand evaluation are handled at showdown.
- Mouse-first controls: `START TABLE`, bot-count buttons, `FOLD`, `CHECK` / `CALL`, `ALL-IN`, `RAISE`, `NEXT HAND`, `MENU` and `F11` fullscreen. Keyboard shortcuts (`SPACE`, `F`, `C`, `R`, `A`, `N`, `M`, `ESC`) still work.
- System TTF typography via `gfx_font_default` (Segoe UI / DejaVu Sans / Helvetica) instead of the bitmap font; the game falls back to `gfx_text` when the TTF backend is unavailable.
- A pre-rendered 1920x1080 canvas: oval table texture with a wooden rail, gold rim, stitched felt and felt speckles under `examples/assets/table/felt.png`.
- 52 generated card textures plus a graphite-and-gold card back under `examples/assets/cards/`, and a custom pointer at `examples/assets/ui/cursor.png` (the OS cursor is hidden while the table is open).
- Staggered deal animation: each card slides from the deck to its seat.
- Chip-stack rendering by denomination (`1000` gold, `500` purple, `100` black, `25` green, `5` red) for the pot and for each seat's committed chips.
- Each bot evaluates its made seven-card hand and folds, calls or raises with a little per-seat aggression; showdowns reveal the cards and print the winning hand category.
- Distinct WAV effects for deal, chips, win, lose and fold under `examples/assets/sounds/`.
- Chip stack persists between sessions in `poker_save.txt`; `PLAY AGAIN` restores 1,000 chips after a bust.

```bash
cd examples
./run_poker.bat
```

`run_poker.bat` switches to the `examples/` directory, puts the installed SDL runtime on `PATH`, rebuilds the example with the LLVM backend and launches it, so it can be started from any directory. `nexus build` also copies the SDL DLLs next to the produced executable, so once `examples/poker_gui.exe` has been built it can be double-clicked directly; keep `assets/` (or the repository root) as the working directory so the textures and sounds resolve.

## Other application demos

- `examples/nexus_focus.nx`: graphical focus/break timer with mouse buttons, sound tone and a small session-count file.
- `examples/nexus_editor.nx`: experimental 2D scene/box editor built on the current ECS, UI and snapshot helpers.
- `examples/nexus_studio.nx`: terminal-menu demo with calculator, guessing-game and mini-battle routines.
- `examples/nexus_paint.nx`: native raster painting tool; documented under [Paint Template](../templates/paint.md).

These examples are meant to show what the compiler/runtime currently supports. Check source and backend requirements before relying on a capability in your own application.
