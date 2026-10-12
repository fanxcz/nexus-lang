# Library Template

Reusable library package template.

## Create

```bash
nexus new MyLib --template library
cd MyLib
nexus build
nexus test
nexus doc
```

## Structure

```
MyLib/
├── nexus.toml
├── src/
│   ├── lib.nx        # Main exports
│   ├── math.nx       # Math module
│   ├── collections.nx
│   └── utils.nx
├── tests/
│   ├── test_math.nx
│   └── test_collections.nx
├── examples/
│   └── basic_usage.nx
├── README.md
├── LICENSE
└── CHANGELOG.md
```

## Library Entry Point

```nx
// src/lib.nx
module my_lib {
    // Re-export all public modules
    export module math
    export module collections
    export module utils
    
    // Version info
    export const VERSION: str = "1.0.0"
    
    // Initialize (optional)
    export fn init() { }
}
```

## Module Example

```nx
// src/math.nx
module math {
    export const PI: f64 = 3.14159265359
    
    export fn add(a: i32, b: i32) -> i32 { a + b }
    export fn mul(a: i32, b: i32) -> i32 { a * b }
    
    export struct Vec2 {
        x: f32,
        y: f32,
    }
    
    export impl Vec2 {
        fn new(x: f32, y: f32) -> Self { Self { x, y } }
        fn length(&self) -> f32 { (self.x*self.x + self.y*self.y).sqrt() }
        fn normalize(&self) -> Self {
            let len = self.length()
            if len > 0 { Self { x: self.x/len, y: self.y/len } }
            else { Self { x: 0, y: 0 } }
        }
    }
}
```

## Tests

```nx
// tests/test_math.nx
import my_lib

#[test]
fn test_add() {
    assert_eq!(my_lib::math::add(2, 3), 5)
}

#[test]
fn test_vec2_length() {
    let v = my_lib::math::Vec2::new(3.0, 4.0)
    assert_approx!(v.length(), 5.0, 0.001)
}

#[test]
fn test_vec2_normalize() {
    let v = my_lib::math::Vec2::new(3.0, 4.0)
    let n = v.normalize()
    assert_approx!(n.length(), 1.0, 0.001)
}
```

## Examples

```nx
// examples/basic_usage.nx
import my_lib

fn main() {
    println("MyLib v{}", my_lib::VERSION)
    
    let sum = my_lib::math::add(10, 20)
    println("10 + 20 = {}", sum)
    
    let v = my_lib::math::Vec2::new(5.0, 12.0)
    println("Length: {}", v.length())  // 13.0
}
```

## Publishing

```bash
# After testing
nexus test
nexus fmt --check
nexus lint
nexus doc

# Publish (future)
nexus pkg publish
```

## Using in Other Projects

```bash
# In consumer project
nexus pkg add ../MyLib
nexus build
```

```nx
// Consumer code
import my_lib

fn main() {
    let v = my_lib::math::Vec2::new(1.0, 1.0)
    println("Diagonal: {}", v.length())  // √2
}
```