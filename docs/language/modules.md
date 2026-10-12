# Modules & Imports

NEXUS uses a file-based module system similar to Rust.

## Module Declaration

```nx
// math.nx - defines module "math"
module math {
    export const PI: f64 = 3.14159265359
    
    export fn sin(x: f64) -> f64 { ... }
    export fn cos(x: f64) -> f64 { ... }
    
    export struct Vec2 {
        x: f32,
        y: f32,
    }
}
```

## Module Files

```
src/
├── main.nx           // Root module
├── math.nx           // math module
├── game/
│   ├── mod.nx        // game module
│   ├── player.nx     // game::player submodule
│   └── enemy.nx      // game::enemy submodule
└── utils.nx          // utils module
```

### `mod.nx` - Re-exports

```nx
// game/mod.nx
module game {
    export module player
    export module enemy
    
    export fn run() { ... }
}
```

## Importing

```nx
// Import entire module
import math
import game

// Use items
math::sin(0.0)
game::run()

// Import with alias
import math as m
m::sin(0.0)

// Selective import
import math { sin, cos, Vec2 }
sin(0.0)  // Direct use

// Import all (use sparingly)
import math::*
```

## Visibility

```nx
module mylib {
    // Public - visible to importers
    export fn public_api() { ... }
    export struct PublicStruct { ... }
    
    // Private - only within this module
    fn internal_helper() { ... }
    struct InternalStruct { ... }
    
    // Pub(crate) - visible within package (future)
    // pub(crate) fn crate_visible() { ... }
}
```

## Standard Library

```nx
// Stdlib modules always available
import std.io
import std.fs
import std.net
import std.json
import std.db
import std.memory
import std.crypto
```

## Package Dependencies

```nx
// nexus.toml
[dependencies]
my-lib = { path = "vendor/my-lib" }

// Usage
import my_lib
my_lib::do_something()
```

## Module Resolution Order

1. **Standard library** (`std.*`)
2. **Local modules** (same package)
3. **Dependencies** (`vendor/*`)

## Circular Imports

Not allowed. Design modules as DAG.

```nx
// ❌ Error: circular dependency
// a.nx: import b
// b.nx: import a
```

## Conditional Modules

```nx
#[cfg(target = "x86_64-windows")]
module platform {
    export fn windows_api() { ... }
}

#[cfg(target = "x86_64-linux")]
module platform {
    export fn linux_api() { ... }
}
```

## Best Practices

1. **One module per file** (or directory with `mod.nx`)
2. **Use `export` explicitly** - default is private
3. **Group related items** in submodules
4. **Re-export in `mod.nx`** for clean public API
5. **Avoid deep nesting** - max 3 levels
6. **Use aliases** for conflicting names

## Example Package Structure

```
my-engine/
├── nexus.toml
├── src/
│   ├── lib.nx              // Root: export all public API
│   ├── math/
│   │   ├── mod.nx          // export module vec, export module mat
│   │   ├── vec.nx          // Vec2, Vec3
│   │   └── mat.nx          // Mat4
│   ├── render/
│   │   ├── mod.nx          // export Sprite, export Texture
│   │   ├── sprite.nx
│   │   └── texture.nx
│   └── ecs/
│       ├── mod.nx          // export World, export Entity
│       ├── world.nx
│       └── components.nx
```