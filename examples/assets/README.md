# Example Assets

This directory contains tracked runtime assets used by NEXUS examples. Check source references before removing or moving shared assets.

## Contents

| Path | Used by |
| --- | --- |
| `beep.wav` | `audio_demo.nx` and `v080_game_framework.nx` |
| `cube.obj` | `model_demo.nx` (experimental OBJ model loader demo) |
| `sprite_sheet.bmp` | `v080_game_framework.nx` (legacy animation sample) |
| `ui/cursor.png` | `poker_gui.nx` — custom pointer |
| `sounds/deal.wav`, `chip.wav`, `fold.wav`, `lose.wav`, `win.wav` | `poker_gui.nx` — table sound effects |

## Regenerate the Poker asset pack

From the repository root:

```sh
python examples/assets/generate_assets.py
```

The generator requires Pillow for image generation and uses Python's standard `wave` module for audio. It regenerates the card textures, table texture, pointer, and Poker sound effects. The card back follows the NEXUS graphite-and-gold identity.

Asset paths in examples are sometimes repository-relative, so run programs from the repository root unless their launcher explicitly changes the working directory. The Poker launcher is `examples/run_poker.bat`.
