# std.memory

Low-level memory allocation and layout utilities.

## Allocation

```nx
import std.memory

// Allocate uninitialized
let ptr = mem_alloc(1024)  // *u8
defer mem_free(ptr)

// Allocate zeroed
let buf = mem_alloc_zero(512)  // *u8, zeroed
defer mem_free(buf)

// Reallocate
let new_ptr = mem_realloc(ptr, 2048)  // May move memory
defer mem_free(new_ptr)

// Aligned allocation
let aligned = mem_alloc_aligned(1024, 64)  // 64-byte aligned
defer mem_free(aligned)
```

## Layout

```nx
struct Layout {
    size: usize,
    align: usize,
}

// Create layout for type
let layout = Layout::new::<Vec2>()
// layout.size == 8, layout.align == 4

// Array layout
let array_layout = Layout::array::<i32>(100)
// size=400, align=4

// Struct layout (manual)
let layout = Layout::from_size_align(24, 8)?
```

## Memory Operations

```nx
// Copy memory (like memcpy)
mem_copy(dst, src, 100)  // dst: *mut u8, src: *u8, len: usize

// Set memory (like memset)
mem_set(ptr, 0, 1024)    // ptr: *mut u8, value: u8, len: usize

// Compare memory
let equal = mem_equal(ptr1, ptr2, 100)  // bool

// Zero memory
mem_zero(ptr, 100)
```

## Type-Safe Allocation

```nx
// Allocate single value
let ptr = mem_alloc_t::<Vec2>()  // *mut Vec2
(*ptr) = Vec2::new(1.0, 2.0)
defer mem_free_t(ptr)

// Allocate array
let slice = mem_alloc_slice::<i32>(100)  // []i32
slice[0] = 42
defer mem_free_slice(slice)

// Box-like (single allocation with drop)
let box = mem_box(Vec2::new(3.0, 4.0))  // Owned, auto-free on scope end
```

## Pointer Utilities

```nx
// Cast between pointer types
let u8_ptr = int_as_ptr(0x1000)  // usize -> *u8
let addr = ptr_as_int(ptr)       // *u8 -> usize

// Offset pointer
let offset_ptr = ptr.add(16)     // *u8 + 16 bytes
let offset_ptr = ptr.sub(8)      // *u8 - 8 bytes

// Pointer arithmetic
let diff = ptr2.offset_from(ptr1)  // isize (elements)

// Null checks
ptr.is_null()
```

## Allocators (Advanced)

```nx
// Default global allocator
let global = mem.global_allocator()

// Arena allocator (fast, bulk free)
let arena = mem.arena_allocator(1024 * 1024)  // 1MB arena
let ptr = arena.alloc(256)
// ... use arena ...
arena.reset()  // Frees all at once

// Pool allocator (fixed-size blocks)
let pool = mem.pool_allocator(64, 1000)  // 64-byte blocks, 1000 count
let ptr = pool.alloc()
pool.free(ptr)
```

## Example

```nx
import std.memory

struct Vec2 { x: f32, y: f32 }

fn main() {
    // Manual allocation
    let layout = Layout::new::<Vec2>()
    let ptr = mem_alloc_aligned(layout.size, layout.align)
    
    // Initialize
    let vec = ptr as *mut Vec2
    *vec = Vec2::new(1.0, 2.0)
    
    println("Vec: {}, {}", (*vec).x, (*vec).y)
    
    // Copy
    let ptr2 = mem_alloc(layout.size)
    mem_copy(ptr2, ptr, layout.size)
    
    // Cleanup
    mem_free(ptr)
    mem_free(ptr2)
}
```