# NEXUS Poker

A standalone, offline Texas Hold'em game written in NEXUS. This is a local single-player game with simulated chips; it has no real-money wagering, accounts, or online multiplayer.

## Features

- One to five computer opponents at a six-seat table.
- No-limit Hold'em rounds: pre-flop, flop, turn and river.
- Fold, check/call, raise, all-in, side pots and split pots.
- Seven-card hand evaluation with kicker comparison.
- Animated dealing, sound effects, a custom cursor and alternate felt themes.
- Bankroll and cosmetic choices saved locally.

## Windows quick start

Download the **NEXUS Poker Windows ZIP** from this repository's [Actions artifacts](https://github.com/fanxcz/nexus-lang/actions/workflows/poker-release.yml) or a tagged [GitHub Release](https://github.com/fanxcz/nexus-lang/releases). Extract the complete ZIP and launch `NEXUS-Poker.exe`. Keep the `assets` folder and runtime DLL files beside the executable.

## Build from source

Install NEXUS 2.1.1 or later, then run from a repository checkout:

```powershell
nexus check examples/poker_gui.nx
cd examples
.\run_poker.bat
```

The launcher builds the game with the LLVM backend and starts it from the `examples` directory so textures and sounds resolve. F11 toggles fullscreen; Esc closes the game.

## Controls

| Action | Mouse | Keyboard |
|---|---|---|
| Fold | Fold button | F |
| Check / call | Check / Call button | C |
| Raise to selected amount | Raise button and slider | R |
| All-in | All-in button | A |
| Deal / next hand | Deal / Next Hand button | Space / N |
| Return to menu | Menu button | M |
| Fullscreen | F11 button | F11 |

## Publishing

The **Package NEXUS Poker for Windows** workflow builds a distributable ZIP and uploads it as a GitHub Actions artifact. To publish a permanent GitHub Release, create and push a tag such as `poker-v1.0.0`; the workflow attaches the Windows ZIP automatically. The same ZIP can be uploaded to itch.io as a downloadable Windows game.

## License

The NEXUS Poker source and project-generated assets are provided under the repository's MIT license. Third-party runtime libraries retain their own licenses; see `THIRD_PARTY_NOTICES.md` in the package.
