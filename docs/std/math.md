# `std.math`

Import `std.math` for scalar helpers shared by gameplay and simulations.

- `math_clamp(value, minimum, maximum) -> f64`: clamps to the inclusive bounds; reversed bounds are normalized.
- `math_min(a, b) -> f64` and `math_max(a, b) -> f64`.
- `math_floor(value) -> i64`: rounds toward negative infinity, including negative fractional values.
- `math_lerp(start, finish, amount) -> f64`: interpolation with amount clamped to 0–1.
- `math_approach(value, target, amount) -> f64`: moves toward the target without stepping past it; negative amount leaves the value unchanged.
- `math_sign(value) -> f64`: returns -1, 0 or 1.

```nx
import std.math

fn main() {
    let dt = math_clamp(0.08, 0.0, 0.05)
    let next = math_approach(3.0, 10.0, 1.5)
    print(dt)
    print(next)
}
```

The module operates on `f64` values and does not currently define vector, matrix or random-number types.
