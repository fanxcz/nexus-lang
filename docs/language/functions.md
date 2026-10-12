# Functions

Functions are first-class citizens in NEXUS.

## Basic Syntax

```nx
fn add(a: i32, b: i32) -> i32 {
    a + b  // Implicit return (last expression)
}

fn greet(name: str) {
    print("Hello, {name}!")
}
```

## Parameters

```nx
// By value (copy)
fn by_value(x: i32) { ... }

// By immutable reference
fn by_ref(x: &i32) { ... }

// By mutable reference
fn by_mut_ref(x: &mut i32) { *x += 1 }

// Multiple parameters
fn draw_rect(x: f32, y: f32, w: f32, h: f32, color: u32) { ... }
```

## Return Values

```nx
// Single return
fn square(x: i32) -> i32 { x * x }

// Multiple returns (tuple)
fn divmod(a: i32, b: i32) -> (i32, i32) {
    (a / b, a % b)
}

let (q, r) = divmod(10, 3)  // q=3, r=1

// Early return
fn find(items: []i32, target: i32) -> Option<usize> {
    for i in 0..items.len() {
        if items[i] == target { return Some(i) }
    }
    None
}
```

## Methods (Associated Functions)

```nx
struct Vec2 {
    x: f32,
    y: f32,
}

impl Vec2 {
    // Constructor (associated function)
    fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
    
    // Method (takes &self)
    fn length(&self) -> f32 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
    
    // Mutable method
    fn normalize(&mut self) {
        let len = self.length()
        if len > 0.0 {
            self.x /= len
            self.y /= len
        }
    }
    
    // Static method
    fn zero() -> Self { Self { x: 0.0, y: 0.0 } }
}

let v = Vec2::new(3.0, 4.0)
v.normalize()
```

## Generic Functions

```nx
fn identity<T>(x: T) -> T { x }

fn max<T: Ord>(a: T, b: T) -> T {
    if a > b { a } else { b }
}

// Multiple type parameters
fn pair<A, B>(a: A, b: B) -> (A, B) { (a, b) }
```

## Function Pointers

```nx
// Type alias
type Callback = fn(i32) -> i32

// Usage
fn apply(f: Callback, x: i32) -> i32 { f(x) }

let double = |x: i32| -> i32 { x * 2 }
apply(double, 5)  // 10
```

## Closures (Planned)

```nx
// Capturing environment (future)
let mut count = 0
let counter = || { count += 1; count }
```

## Defer

```nx
fn read_file(path: str) -> Result<str, Error> {
    let file = fs_open(path)?
    defer fs_close(file)  // Runs on scope exit (success or error)
    
    let content = fs_read_all(file)?
    Ok(content)
}
```

## Variadic (FFI only)

```nx
extern "C" {
    fn printf(format: *const i8, ...) -> i32
}
```

## Inline

```nx
#[inline]
fn small_fn(x: i32) -> i32 { x + 1 }

#[inline(always)]
fn hot_path(x: i32) -> i32 { x * 2 }
```

## Attributes

```nx
#[no_mangle]
fn exported_function() { ... }

#[export_name = "my_name"]
fn with_custom_name() { ... }

#[cold]
fn error_path() { ... }
```

## Main Function

```nx
// Standard entry point
fn main() { ... }

// With args (future)
fn main(args: []str) -> i32 { 0 }

// Async main (future)
// async fn main() { ... }
```