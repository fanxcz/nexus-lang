# `std.collision2d`

Import `std.collision2d` for simple collision predicates and movement against the solid cells of `std.tilemap`.

- `collision2d_aabb(ax, ay, aw, ah, bx, by, bw, bh) -> bool`: overlap test for axis-aligned boxes.
- `collision2d_circle(ax, ay, ar, bx, by, br) -> bool`: overlap test for circles represented by center and radius.
- `collision2d_map_overlaps(map, map_width, map_height, tile_size, x, y, box_width, box_height) -> bool`.
- `collision2d_move_x(..., delta) -> f64` and `collision2d_move_y(..., delta) -> f64`: resolve a proposed move along one axis.

```nx
import std.tilemap
import std.collision2d

fn main() {
    let map = tilemap_create(20, 12)
    tilemap_set(map, 20, 12, 5, 8, 1)
    let safe_x = collision2d_move_x(map, 20, 12, 32.0,
                                    32.0, 256.0, 24.0, 28.0, 80.0)
    print(safe_x)
}
```

Move X then Y and compare the returned position to the desired position to detect an impact. A starting box must not already overlap a solid tile. Substeps are capped at 10,000; the helper targets ordinary frame movement, not arbitrarily large teleports. Out-of-map cells are treated as empty. This is not a rigid-body physics simulation.
