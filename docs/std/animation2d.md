# `std.animation2d`

Frame-index calculations are kept separate from textures and rendering.

- `animation2d_frame_at(elapsed_seconds, frame_count, frames_per_second, looped) -> i64`: computes a sprite-sheet frame from elapsed time.
- `animation2d_next_frame(current, frame_count, looped) -> i64`: advances by one frame, looping to zero or clamping to the final frame.
- `animation2d_ease_in(amount)`, `animation2d_ease_out(amount)`, `animation2d_ease_in_out(amount)`, `animation2d_smoothstep(amount)`: easing functions with amounts clamped to 0–1.
- `animation2d_ping_pong(elapsed_seconds, duration) -> f64`: repeating 0-to-1-to-0 triangle wave.

Invalid frame counts and non-positive frame rates return frame 0. Negative elapsed time is treated as zero. Non-looping animations hold the final frame.

```nx
import std.animation2d

fn main() {
    let frame = animation2d_frame_at(0.35, 8, 12.0, true)
    print(frame)
}
```

Use the returned frame with `sprite2d_draw_frame` from [`std.sprite2d`](sprite2d.md). This module does not own per-entity clocks or animation state; the game stores elapsed time for each entity.
