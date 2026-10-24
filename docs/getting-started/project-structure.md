# Project Structure

NEXUS keeps generated projects simple. The recommended layout has six top-level folders; only `src/` is required for a basic program. Add assets, tests, dependencies, build output, and documentation when the project needs them.

## Standard Layout

```text
project-root/
├── nexus.toml              # Project manifest
├── README.md               # Project overview
├── .gitignore              # Files excluded from version control
├── nexus.lock              # Optional dependency lockfile
├── src/                    # Required: source files
│   ├── main.nx             # Entry point (or [build].entry)
│   └── *.nx                # Additional modules
├── assets/                 # Optional: runtime assets
│   ├── images/
│   ├── audio/
│   ├── fonts/
│   └── shaders/
├── tests/                  # Optional: test programs
│   └── *.nx
├── vendor/                 # Optional: local packages (gitignored)
│   └── <package-name>/
│       ├── nexus.toml
│       └── src/
├── build/                  # Optional: generated output (gitignored)
└── docs/                   # Optional: project documentation
```

The six top-level folders are `src/`, `assets/`, `tests/`, `vendor/`, `build/`, and `docs/`. Start with `src/main.nx` and add the other folders only when needed.
## `nexus.toml` Reference

### `[package]` Section

```toml
[package]
name = "my-project"           # Required: Package name (lowercase, hyphens)
version = "1.0.0"             # Required: SemVer version
edition = "2026"              # Required: Language edition
description = "..."           # Optional: Short description
authors = ["Name <email>"]    # Optional: Author list
license = "MIT"               # Optional: SPDX license identifier
repository = "https://..."    # Optional: Repository URL
homepage = "https://..."      # Optional: Project homepage
keywords = ["game", "2d"]     # Optional: Search keywords
categories = ["games"]        # Optional: Categories
readme = "README.md"          # Optional: README path
```

### `[build]` Section

```toml
[build]
entry = "src/main.nx"         # Entry point (default: src/main.nx)
optimization = "speed"        # "speed" | "size" | "debug" (default: speed)
debug_symbols = true          # Include debug info (default: true for debug)
target_cpu = "native"         # Target CPU (default: native)
linker = "lld"                # Linker: "lld" | "mold" | "gold" | "default"
rpath = true                  # Embed rpath (default: true)
```

### `[targets]` Section

```toml
[targets]
default = "x86_64-linux"      # Default target triple

# Named targets for cross-compilation
windows = "x86_64-windows"
macos = "x86_64-macos"
linux_arm64 = "aarch64-linux"
web = "wasm32-unknown-unknown"  # Experimental
```

### `[dependencies]` Section

```toml
[dependencies]
# Local path dependency
my-lib = { path = "vendor/my-lib" }

# Git dependency (future)
# my-lib = { git = "https://github.com/user/repo", rev = "v1.0.0" }

# Registry dependency (future)
# my-lib = "1.2.3"
```

### `[dev-dependencies]` Section

```toml
[dev-dependencies]
test-utils = { path = "vendor/test-utils" }
```

### `[profile.*]` Sections

```toml
[profile.debug]
optimization = "debug"
debug_symbols = true
overflow_checks = true

[profile.release]
optimization = "speed"
debug_symbols = false
lto = "thin"          # "off" | "thin" | "full"
strip = true
```

## Source Files

### Module Structure

```
src/
├── main.nx           # Main entry point
├── game/             # Game module
│   ├── mod.nx        # Module definition
│   ├── player.nx
│   └── enemy.nx
├── render/           # Render module
│   ├── mod.nx
│   └── sprite.nx
└── utils.nx          # Single-file module
```

### Module Declaration (`mod.nx`)

```nx
// game/mod.nx
module game {
    export fn spawn_player(x: f32, y: f32) -> Entity
    export fn update_player(e: Entity, dt: f32)
    export struct Player { ... }
}
```

### Importing Modules

```nx
// Import standard library
import std.io
import std.fs

// Import local module
import game
import render

// Import with alias
import std.net as net

// Selective import
import std.json { parse, stringify }
```

## Assets

Assets in `assets/` are copied to the build output and accessible at runtime:

```nx
// Load image from assets/images/
let texture = gfx_load_texture("assets/images/player.png")

// Load sound from assets/audio/
let sound = audio_load_sound("assets/audio/jump.wav")

// Load font from assets/fonts/
let font = gfx_load_font("assets/fonts/main.ttf", 24)
```

Use `nexus assets` command to manage assets:

```bash
nexus assets list           # List all assets
nexus assets add sprite.png # Add asset
nexus assets remove old.png # Remove asset
```

## Build Output

The compiler writes generated artifacts into the project's `build/` directory (or the configured artifact path). A native build produces an executable and may keep the intermediate LLVM IR file used by the native backend. Windows executables use the `.exe` suffix.

```
build/
├── debug/                 # Debug build artifacts (when using a project manifest)
├── release/               # Optimized build artifacts (when using --release)
└── <generated files>      # Native executable and intermediate IR
```

The compiler links the native runtime into generated applications; the source repository's `src/runtime/`, `stdlib/`, and `examples/templates/` directories are not copied into every project's build output.

## Ignored Files

Add to `.gitignore`:

```
# Build output
build/
*.o
*.obj
*.exe
*.pdb

# Package cache
vendor/
nexus.lock

# IDE
.vscode/
.idea/
*.swp
*.swo

# OS
.DS_Store
Thumbs.db
```

## Multi-Project Workspaces

For larger projects, use a workspace layout:

```
workspace/
├── nexus-workspace.toml   # Workspace config
├── engine/                # Engine library
│   ├── nexus.toml
│   └── src/
├── game/                  # Game executable
│   ├── nexus.toml
│   └── src/
└── tools/                 # Development tools
    ├── nexus.toml
    └── src/
```

Workspace config:

```toml
# nexus-workspace.toml
[workspace]
members = ["engine", "game", "tools"]
default_target = "x86_64-linux"
```