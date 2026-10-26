# `std.tilemap`

This module owns the grid data model only. Rendering and collision are separate modules: see [`std.tilemap_render`](tilemap_render.md) and [`std.collision2d`](collision2d.md).

- `tilemap_create(width, height) -> i64`: returns a zero-filled, row-major integer-list handle. Returns 0 for invalid dimensions or maps larger than 262,144 cells.
- `tilemap_destroy(map) -> bool`: releases the underlying integer-list handle and returns false for an invalid/already-destroyed handle.
- `tilemap_dispose(map)`: void-returning companion for `defer tilemap_dispose(map)`.
- `tilemap_valid(map, width, height) -> bool`: checks dimensions and handle length.
- `tilemap_get(map, width, height, x, y) -> i64`: out-of-range reads return 0.
- `tilemap_set(map, width, height, x, y, tile)`: out-of-range writes are ignored.
- `tilemap_try_set(map, width, height, x, y, tile) -> bool`: returns false for invalid maps or coordinates instead of silently ignoring the write.
- `tilemap_fill(map, width, height, tile) -> bool`: sets every cell to the supplied tile ID.
- `tilemap_fill_rect(map, width, height, x, y, rect_width, rect_height, tile) -> bool`: fills only if the full rectangle is inside the map; invalid/out-of-bounds rectangles return false.
- `tilemap_count(map, width, height, tile) -> i64`: counts cells with the requested tile ID.
- `tilemap_fill(map, width, height, tile) -> bool`: sets every cell to the supplied tile ID.
- `tilemap_fill_rect(map, width, height, x, y, rect_width, rect_height, tile) -> bool`: fills a rectangular region only when the full requested area fits inside the map; invalid/out-of-bounds rectangles return false.
- `tilemap_count(map, width, height, tile) -> i64`: counts cells with the specified tile ID.

Tile ID 0 is empty. The core module does not dictate what nonzero IDs mean; the collision helper treats positive IDs as solid, while the debug renderer chooses colors for IDs 1–4 and a default for other positive IDs.

```nx
import std.tilemap

fn main() {
    let map = tilemap_create(20, 12)
    let _ = tilemap_fill_rect(map, 20, 12, 0, 10, 20, 2, 1)
    tilemap_set(map, 20, 12, 3, 8, 2)
    print(tilemap_get(map, 20, 12, 3, 8))
    print(tilemap_count(map, 20, 12, 1))
}
```

Map storage is allocated through the runtime's integer-list API. Call `tilemap_destroy` when ownership ends, or schedule `defer tilemap_dispose(map)` immediately after creation so early returns do not leak the handle. Creation reserves capacity and checks allocation/push results, returning 0 if initialization fails. The integer handle is opaque and must not be used after destruction.
