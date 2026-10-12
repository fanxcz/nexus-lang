# Game Template

The current `game` template copies the checked-in `examples/templates/game.nx` source into `src/main.nx`. It is a single-file graphical survivor mini-game, not a multi-file engine framework. The separate `examples/neon_survivor.nx` file is another example and is not the source selected for this template by the current source-tree lookup.

## Create and run

```bash
nexus new MyGame --template game
cd MyGame
nexus check
nexus build
nexus run
```

## Included behavior

The template source creates a native graphics window and implements a start/pause/game-over loop, movement, enemies, projectiles, health/score/wave state and drawing using the built-in `gfx_*` API. Its state is deliberately explicit in one source file because `nexus new` copies a template file instead of generating a multi-module project.

## Extend it

Open `src/main.nx` and edit the game loop, movement/arena limits, enemy positions, projectile logic, collision checks and drawing. The runtime includes 2D primitives, keyboard/mouse state and simple collision/ECS helpers. It does not provide a general physics world, pathfinding, networking or a complete AI system.
