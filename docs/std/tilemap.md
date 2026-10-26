# `std.tilemap`

This module owns the grid data model only. Rendering and collision are separate modules: see [`std.tilemap_render`](tilemap_render.md) and [`std.collision2d`](collision2d.md).

- `tilemap_create(width, height) -> i64`: returns a zero-filled, row-major integer-list handle. Returns 0 for invalid dimensions or maps larger than 262,144 cells.
- `tilemap_valid(map, width, height) -> bool`: checks dimensions and handle length.
- `tilemap_get(map, width, height, x, y) -> i64`: out-of-range reads return 0.
- `tilemap_set(map, width, height, x, y, tile)`: out-of-range writes are ignored.

Tile ID 0 is empty. The core module does not dictate what nonzero IDs mean; the collision helper treats positive IDs as solid, while the debug renderer chooses colors for IDs 1–4 and a default for other positive IDs.

```nx
import std.tilemap

fn main() {
    let map = tilemap_create(20, 12)
    tilemap_set(map, 20, 12, 3, 8, 1)
    print(tilemap_get(map, 20, 12, 3, 8))
}
```

Map handles use the runtime's integer-list storage and currently have process lifetime; there is no dedicated tilemap destroy API.
