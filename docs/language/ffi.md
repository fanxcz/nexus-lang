# Foreign Function Interface (FFI)

NEXUS provides seamless C interoperability via `extern "C"` blocks.

## Basic FFI

```nx
// Declare C functions
extern "C" {
    fn printf(format: *const i8, ...) -> i32
    fn malloc(size: usize) -> *mut u8
    fn free(ptr: *mut u8)
    fn strlen(s: *const i8) -> usize
}

// Call from NEXUS
fn main() {
    unsafe {
        printf(c"Hello from C!\n")
        let ptr = malloc(100)
        free(ptr)
    }
}
```

## Calling Conventions

```nx
extern "C" { }      // C calling convention (default for extern)
extern "system" { } // System default (stdcall on Windows x86)
extern "cdecl" { }  // Explicit cdecl
extern "fastcall" { } // Fastcall
```

## Type Mapping

| C Type | NEXUS Type |
|--------|------------|
| `int8_t` / `char` | `i8` / `u8` |
| `int16_t` / `short` | `i16` |
| `int32_t` / `int` | `i32` |
| `int64_t` / `long long` | `i64` |
| `uint8_t` / `unsigned char` | `u8` |
| `uint32_t` / `unsigned int` | `u32` |
| `float` | `f32` |
| `double` | `f64` |
| `void*` | `*mut u8` / `*u8` |
| `const char*` | `*const i8` |
| `char*` | `*mut i8` |
| `size_t` | `usize` |
| `ptrdiff_t` | `isize` |
| `bool` | `bool` (0/1) |
| `struct` | `struct` (same layout) |

## Strings

```nx
// C string (null-terminated)
extern "C" {
    fn puts(s: *const i8) -> i32
}

// NEXUS string to C string
let s = "Hello"
unsafe {
    puts(s.as_c_str())  // Auto-adds null terminator
}

// C string to NEXUS
unsafe {
    let c_str = some_c_function()
    let nx_str = str_from_c_str(c_str)  // Copies, validates UTF-8
}
```

## Structs

```nx
// C struct
// typedef struct { int x, y; } Point;

extern "C" {
    fn translate_point(p: *mut Point, dx: i32, dy: i32)
}

// NEXUS mirror (must match layout exactly)
#[repr(C)]
struct Point {
    x: i32,
    y: i32,
}

fn main() {
    let mut p = Point { x: 10, y: 20 }
    unsafe {
        translate_point(&mut p as *mut Point, 5, 5)
    }
    print("Point: {p.x}, {p.y}")  // 15, 25
}
```

## Callbacks

```nx
// C callback type
// typedef void (*Callback)(int data);

extern "C" {
    fn register_callback(cb: fn(i32), user_data: *mut u8)
    fn trigger_callbacks()
}

// NEXUS callback
fn my_callback(data: i32) {
    print("Callback called with: {data}")
}

fn main() {
    unsafe {
        register_callback(my_callback, null)
        trigger_callbacks()
    }
}
```

## Variadic Functions

```nx
extern "C" {
    fn printf(format: *const i8, ...) -> i32
    fn sprintf(buf: *mut i8, format: *const i8, ...) -> i32
}

fn main() {
    unsafe {
        printf(c"Value: %d\n", 42)
        printf(c"Float: %.2f\n", 3.14159)
    }
}
```

## Linking Libraries

```bash
# In build.zig / CMakeLists.txt / nexus.toml (future)
# Link with: -lm -lpthread -lssl -lcrypto
```

## Complete Example: SQLite

```nx
// Minimal SQLite binding
extern "C" {
    fn sqlite3_open(filename: *const i8, db: *mut *mut u8) -> i32
    fn sqlite3_close(db: *mut u8) -> i32
    fn sqlite3_exec(
        db: *mut u8,
        sql: *const i8,
        callback: fn(*mut u8, i32, **mut i8, **mut i8) -> i32,
        user_data: *mut u8,
        errmsg: *mut *mut i8
    ) -> i32
}

fn main() {
    var db: *mut u8 = null
    unsafe {
        let rc = sqlite3_open(c"test.db", &db)
        if rc != 0 { panic("Failed to open DB") }
        defer sqlite3_close(db)
        
        let sql = c"CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT);"
        let rc = sqlite3_exec(db, sql, null, null, null)
        if rc != 0 { panic("Create table failed") }
        
        print("SQLite works!")
    }
}
```

## Safety

```nx
// All FFI calls require unsafe block
unsafe {
    // Dereference raw pointers
    // Call extern functions
    // Cast between pointers/integers
    // Access mutable statics
}

// Safe wrapper pattern
fn safe_malloc(size: usize) -> Result<*mut u8, str> {
    unsafe {
        let ptr = malloc(size)
        if ptr.is_null() { Err("OOM") }
        else { Ok(ptr) }
    }
}
```

## Platform-Specific

```nx
#[cfg(target = "x86_64-windows")]
extern "system" {
    fn LoadLibraryA(name: *const i8) -> *mut u8
    fn GetProcAddress(module: *mut u8, name: *const i8) -> *mut u8
}

#[cfg(target = "x86_64-linux")]
extern "C" {
    fn dlopen(filename: *const i8, flags: i32) -> *mut u8
    fn dlsym(handle: *mut u8, symbol: *const i8) -> *mut u8
}
```

## Best Practices

1. **Wrap unsafe in safe functions** - expose safe API
2. **Use `#[repr(C)]`** for structs matching C layout
3. **Validate pointers** before dereferencing
4. **Manage lifetimes** - don't let C hold refs to NEXUS memory
5. **Test on all targets** - calling conventions differ