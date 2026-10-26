# Poker handoff

This folder exists so the poker game can be handed to another agent or another
machine without cloning the whole history.

- `POKER-AGENT-PROMPT.md` — the self-contained task prompt (in Russian).
- `poker_gui.nx` — byte-identical snapshot of `examples/poker_gui.nx` at the
  time of handoff (NEXUS 2.1.0).

The canonical source is always [`examples/poker_gui.nx`](../examples/poker_gui.nx).
When you finish a new round of work, edit the canonical file and refresh this
snapshot only if you still need a standalone copy.

Assets used by the game:

- `examples/assets/cards/` — 52 cards + back
- `examples/assets/table/felt.png` — 1920×1080 table
- `examples/assets/ui/cursor.png` — arrow pointer
- `examples/assets/sounds/` — deal, chip, win, lose, fold

Build and check commands are in the prompt. `ctest` currently passes 36/36.
