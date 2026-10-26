# Example Assets

This directory contains tracked runtime assets shared by multiple NEXUS examples. Keep files here unless all source and documentation references have been checked before removal.

## Contents

| Path | Used by |
| --- | --- |
| `beep.wav` | `audio_demo.nx` and `v080_game_framework.nx` |
| `cube.obj` | `model_demo.nx` (experimental OBJ model loader demo) |
| `sprite_sheet.bmp` | `v080_game_framework.nx` (legacy animation sample) |
| `cards/card_*.png`, `cards/card_back.png` | `poker_gui.nx` — 52 card faces and the back texture |
| `table/felt.png` | `poker_gui.nx` — pre-rendered table |
| `ui/cursor.png` | `poker_gui.nx` — custom arrow cursor |
| `sounds/deal.wav`, `chip.wav`, `fold.wav`, `lose.wav`, `win.wav` | `poker_gui.nx` — table sound effects |

## Regenerate the Poker asset pack

From the repository root:

```sh
python examples/assets/generate_assets.py
```

The generator requires Pillow for image generation and uses Python's standard `wave` module for audio. Its current entry point regenerates card textures, the table texture, and Poker sound effects. It does not rewrite `ui/cursor.png`.

Asset paths in examples are sometimes repository-relative, so run programs from the repository root unless their launcher explicitly changes the working directory. The Poker launcher is `examples/run_poker.bat`.
