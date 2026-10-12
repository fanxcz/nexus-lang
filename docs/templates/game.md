# Game Template

2D game starter template.

## Create

```bash
nexus new MyGame --template game
cd MyGame
nexus build
nexus run
```

## Structure

```
MyGame/
├── nexus.toml
├── src/
│   ├── main.nx       # Entry point
│   ├── player.nx     # Player logic
│   ├── enemy.nx      # Enemy logic
│   └── game.nx       # Game state
├── assets/
│   ├── sprites/
│   ├── audio/
│   └── fonts/
└── tests/
```

## Features Included

- Player movement (WASD/Arrow keys)
- Basic enemy AI
- Collision detection
- Score system
- Game over / restart
- Sound effects
- Particle effects (explosions)

## Customization

```nx
// src/game.nx - Modify constants
const PLAYER_SPEED = 300.0
const ENEMY_SPEED = 150.0
const ENEMY_SPAWN_RATE = 2.0  // per second
const MAX_ENEMIES = 10
```

## Adding Assets

```bash
# Add sprite
nexus assets add assets/sprites/player.png

# Add sound
nexus assets add assets/audio/jump.wav

# Use in code
let player_tex = gfx_load_texture("assets/sprites/player.png")
let jump_sfx = audio_load_sound("assets/audio/jump.wav")
```