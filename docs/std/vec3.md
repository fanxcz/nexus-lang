# `std.vec3`

Import `std.vec3` for three-dimensional spatial math used by models, cameras and gameplay.

```nx
import std.vec3

fn main() {
    let right = vec3(1.0, 0.0, 0.0)
    let up = vec3(0.0, 1.0, 0.0)
    let forward = vec3_cross(right, up)
    print(forward.z)
}
```

The `Vec3` structure exposes `x`, `y` and `z` fields, all `f64`. Helpers include `vec3_add`, `vec3_sub`, `vec3_scale`, `vec3_dot`, `vec3_cross`, `vec3_length`, `vec3_distance`, `vec3_normalize` and `vec3_lerp`. Normalizing a near-zero vector safely returns `(0, 0, 0)`.
