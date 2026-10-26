# std.physics2d

Simple platformer-oriented math and axis-separated collision helpers. This is
not a rigid-body solver; it does not implement contact manifolds, impulses,
rotation, joints, or a physics world.

- physics2d_apply_gravity(vy, gravity, dt) -> f64
- physics2d_integrate_x(x, vx, dt) -> f64
- physics2d_integrate_y(y, vy, dt) -> f64
- physics2d_clamp_fall(vy, max_fall) -> f64
- physics2d_jump_velocity(gravity, jump_height) -> f64: negative upward launch speed for y-down coordinates.
- physics2d_apply_friction(velocity, friction, dt) -> f64: moves velocity towards zero without reversing it.
- physics2d_clamp_vertical(vy, max_rise, max_fall) -> f64
- physics2d_move_x(...) and physics2d_move_y(...): resolve movement on one axis against positive solid tile IDs.
- physics2d_on_ground(...): checks for a solid tile directly below the entity's AABB.

Pass frame delta time in seconds. Clamp unusually large delta times after pauses/window stalls. Move horizontally and vertically separately; set vertical velocity to zero when a resolved move differs from the desired position. Coordinates grow downwards, so gravity/fall velocity is positive and jump velocity is negative.

A complete, asset-free starting example is examples/game2d_starter.nx. It uses tilemap storage/rendering, collision, platformer physics, Vec2 camera helpers, animation curves and the core engine2d wrapper.
