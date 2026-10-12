# Control Flow

## If Expressions

```nx
// Statement
if condition {
    // ...
} else if other_condition {
    // ...
} else {
    // ...
}

// Expression (returns value)
let max = if a > b { a } else { b }

// Ternary-like (single line)
let status = if ok { "success" } else { "failure" }
```

## While Loops

```nx
// Standard while
while condition {
    // ...
}

// While with label (for break/continue)
'outer: while true {
    while true {
        break 'outer  // Breaks outer loop
    }
}
```

## For Loops

```nx
// Range (inclusive start, exclusive end)
for i in 0..10 {          // 0 to 9
    // ...
}

for i in 1..=10 {         // 1 to 10 (inclusive)
    // ...
}

// Iterator over slice/array
let items = [1, 2, 3, 4, 5]
for item in items {
    print(item)
}

// With index
for (i, item) in items.enumerate() {
    print("{i}: {item}")
}

// Reverse
for i in (0..10).rev() {  // 9 down to 0
    // ...
}
```

## Loop (Infinite)

```nx
loop {
    // ...
    if done { break }
    
    if skip { continue }
}
```

## Match Expressions

```nx
// Basic match
let result = match value {
    0 => "zero",
    1 => "one",
    2 => "two",
    _ => "other",  // Wildcard (required for non-exhaustive)
};

// Match with guards
match n {
    x if x < 0 => "negative",
    0 => "zero",
    x if x > 100 => "large",
    _ => "small",
}

// Match with binding
match option {
    Some(value) => print("Got: {value}"),
    None => print("Nothing"),
}

// Match struct/enum
match event {
    Event::KeyPress(c) => print("Key: {c}"),
    Event::MouseClick { x, y } => print("Click at {x},{y}"),
    Event::Quit => break,
}

// Match returns value
let description = match status {
    Status::Ok => "Success",
    Status::Error(msg) => "Failed: {msg}",
    Status::Pending => "Waiting...",
};
```

## Break / Continue

```nx
// Break from loop
for i in 0..100 {
    if i == 50 { break }
}

// Continue to next iteration
for i in 0..10 {
    if i % 2 == 0 { continue }
    print(i)  // Prints odd numbers only
}

// Labeled break/continue
'outer: for i in 0..10 {
    for j in 0..10 {
        if i == 5 && j == 5 { break 'outer }
    }
}
```

## Pattern Matching Details

### Literal Patterns
```nx
match x {
    0 => "zero",
    1 | 2 | 3 => "small",
    _ => "other",
}
```

### Range Patterns
```nx
match score {
    0..=50 => "F",
    51..=70 => "C",
    71..=90 => "B",
    91..=100 => "A",
    _ => "Invalid",
}
```

### Struct Patterns
```nx
struct Point { x: i32, y: i32 }

match point {
    Point { x: 0, y: 0 } => "origin",
    Point { x, y: 0 } => "on x-axis",
    Point { x: 0, y } => "on y-axis",
    Point { x, y } => "({x}, {y})",
}
```

### Enum Patterns
```nx
enum Shape {
    Circle(f32),
    Rectangle(f32, f32),
    Square(f32),
}

match shape {
    Shape::Circle(r) => "Circle radius {r}",
    Shape::Rectangle(w, h) => "Rect {w}x{h}",
    Shape::Square(s) => "Square {s}",
}
```

### Reference Patterns
```nx
match &value {
    &x => print("Got reference to {x}"),
    &&x => print("Got double reference"),
}
```

## Control Flow as Expressions

All control flow constructs return values:

```nx
// If expression
let x = if cond { 1 } else { 2 }

// Loop expression (with break value)
let found = loop {
    if condition { break value }
};

// Match expression
let result = match opt { Some(v) => v, None => 0 };

// Block expression
let y = {
    let temp = compute()
    temp * 2
};
```

## Error Propagation

```nx
fn read_and_parse(path: str) -> Result<i32, Error> {
    let content = fs_read_to_string(path)?  // Early return on Err
    let num = content.parse()?              // Early return on Err
    Ok(num)
}
```