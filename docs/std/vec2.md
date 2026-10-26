# `std.vec2`

Import `std.vec2` for the `Vec2` value and two-dimensional vector operations.

```nx
import std.vec2

fn main() {
    let start = vec2(10.0, 20.0)
    let target = vec2(100.0, 80.0)
    let direction = vec2_normalize(vec2_sub(target, start))
    let next = vec2_move_towards(start, target, 5.0)
    print(direction.x)
    print(next.x)
}
```

The `Vec2` structure has `x` and `y` fields of type `f64`. Available helpers include `vec2_add`, `vec2_sub`, `vec2_scale`, `vec2_dot`, `vec2_length`, `vec2_distance`, `vec2_normalize`, `vec2_perpendicular`, `vec2_lerp` and `vec2_move_towards`. Normalizing a near-zero vector safely returns `(0, 0)`.
