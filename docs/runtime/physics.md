# Physics Collision Helpers

The current runtime provides two collision predicates only. It is **not** a full physics engine; it has no exposed world simulation, rigid bodies, gravity, force integration, fixtures, contact listeners or penetration resolution.

## Axis-aligned rectangles

`physics_aabb(ax, ay, aw, ah, bx, by, bw, bh)` checks whether two axis-aligned rectangles overlap.

```nx
let player_x = 100.0
let player_y = 140.0
let enemy_x = 120.0
let enemy_y = 150.0
let hit = physics_aabb(player_x, player_y, 32.0, 32.0,
                      enemy_x, enemy_y, 28.0, 28.0)
print(str_bool(hit))
```

## Circles

`physics_circle(ax, ay, ar, bx, by, br)` treats each point as a circle center and each radius as a distance. It returns true if the circles overlap or touch.

```nx
let hit = physics_circle(100.0, 100.0, 18.0, 125.0, 100.0, 12.0)
print(str_bool(hit))
```

Store position and velocity in your program and call these helpers when checking overlap; they do not move or resolve objects.
