# Memory

Runtime memory management utilities.

## Arena Allocator

```nx
import runtime.memory

fn main() {
    // Create arena (1MB)
    let arena = mem_arena_create(1024 * 1024)
    defer mem_arena_destroy(arena)
    
    // Fast allocations
    let ptr1 = mem_arena_alloc(arena, 256)
    let ptr2 = mem_arena_alloc(arena, 512)
    
    // Reset all at once (frees everything)
    mem_arena_reset(arena)
}
```

## Pool Allocator

```nx
// Fixed-size blocks
let pool = mem_pool_create(64, 1000)  // 64-byte blocks, 1000 count
defer mem_pool_destroy(pool)

let ptr = mem_pool_alloc(pool)
mem_pool_free(pool, ptr)
```

## Frame Allocator

```nx
// Per-frame allocations (auto-freed each frame)
while !gfx_should_close() {
    let ptr = mem_frame_alloc(256)
    // Automatically freed at end of frame
}
```

## Tracking

```nx
// Debug: track allocations
mem_track_enable(true)

let ptr = mem_alloc(100)
mem_free(ptr)

mem_track_report()  // Prints leaks if any
mem_track_disable()
```

## Stats

```nx
let stats = mem_stats()
println("Allocated: {} KB", stats.allocated / 1024)
println("Peak: {} KB", stats.peak / 1024)
println("Allocations: {}", stats.count)
```