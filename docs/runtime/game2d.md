# NEXUS 2D game helpers and tilemaps

The small convenience layer in `stdlib/engine2d.nx` wraps existing runtime calls for frame setup, timing, keyboard input, shapes/text, textures, sprite-sheet frames, camera, AABB overlap and basic sound playback. It is not a separate rendering backend.

## Tilemap module

Import `std.tilemap` to use a grid backed by the runtime's integer-list handle:

- `tilemap_create(width, height) -> i64` creates a zero-filled map, or returns `0` for invalid/oversized dimensions. The current maximum is 262,144 cells.
- `tilemap_get(map, width, height, x, y) -> i64` and `tilemap_set(..., tile)` read/write a tile. Out-of-range reads return `0`; out-of-range writes are ignored.
- Tile ID `0` means empty. Any positive tile ID is solid for collision. `tilemap_draw` uses colored rectangles for IDs 1–4 and a default blue for other positive IDs; it draws only the portion intersecting the supplied world-space viewport.
- `tilemap_collides_box` tests an AABB against solid cells.
- `tilemap_move_x` and `tilemap_move_y` move one axis at a time, using bounded substeps and refinement to prevent ordinary per-frame movement from tunnelling through tiles.

Example:

```nx
import std.tilemap

fn main() {
    let map = tilemap_create(20, 12)
    tilemap_set(map, 20, 12, 3, 8, 1)
    let next_x = tilemap_move_x(map, 20, 12, 32.0,
                                32.0, 256.0, 24.0, 28.0, 80.0)
    print(next_x)
}
```

A game should update X and Y separately. Compare the resolved position with the intended position to detect an impact; when vertical movement is stopped while velocity is downward, mark the character as grounded and reset vertical velocity.

The helper layer uses a fixed grid and treats all nonzero tiles as solid; it does not yet parse Tiled maps, support per-tile collision flags, or provide animated tiles / texture atlases. The integer-list runtime uses process-lifetime handles, so maps do not currently have a dedicated destroy operation.

## Engine/runtime limitations

AABB overlap in `physics_aabb` remains a predicate, not a rigid-body solver. The tilemap movement helpers are axis-separated 2D collision utilities, not a general physics world. Camera behavior depends on the active graphics path, and audio/image formats depend on optional runtime libraries.

The self-contained starter at [`examples/game2d_platformer.nx`](https://github.com/fanxcz/nexus-lang/blob/main/examples/game2d_platformer.nx) demonstrates tilemaps, gravity, jumping, collision resolution, frame-time clamping and drawing without external assets.
