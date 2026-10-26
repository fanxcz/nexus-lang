# `std.math`

Import `std.math` for scalar helpers shared by gameplay and simulations.
NEXUS also provides floating-point math built-ins that can be called directly
from any module; they do not require a separate import:

- `sqrt(x)`, `sin(x)`, `cos(x)`, and `tan(x)` accept and return `f64`.
- `atan2(y, x)` returns the angle in radians using the conventional
  two-argument arctangent order.
- `floor(x)` and `ceil(x)` return rounded `f64` values. Use
  `math_floor(x)` when you specifically need an `i64` rounded down.
- `pow(base, exponent)` calculates a floating-point power.

Angles are in radians. These built-ins are implemented by the native runtime
and available to the LLVM compilation backend.

- `math_clamp(value, minimum, maximum) -> f64`: clamps to the inclusive bounds; reversed bounds are normalized.
- `math_min(a, b) -> f64` and `math_max(a, b) -> f64`.
- `math_floor(value) -> i64`: rounds toward negative infinity, including negative fractional values.
- `math_lerp(start, finish, amount) -> f64`: interpolation with amount clamped to 0–1.
- `math_approach(value, target, amount) -> f64`: moves toward the target without stepping past it; negative amount leaves the value unchanged.
- `math_sign(value) -> f64`: returns -1, 0 or 1.
- `math_sin(radians)`, `math_cos(radians)`, `math_tan(radians)`,
  `math_atan2(y, x)`, and `math_pow(base, exponent)` are namespaced wrappers
  around the core floating-point built-ins.
- `math_floor_f64(value)` and `math_ceil_f64(value)` return rounded
  `f64` values; `math_floor(value)` retains its existing `i64` result.

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
