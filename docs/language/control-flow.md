# Control Flow

The current parser supports `if` / `else`, `while`, `for ... in`, `break`, `continue`, and statement-form `match`. Control-flow constructs are statements rather than general value-producing expressions.

## If / else

```nx
fn main() {
    let value = 42
    if value > 10 {
        print("large")
    } else if value == 10 {
        print("ten")
    } else {
        print("small")
    }
}
```

## While

```nx
let mut count: i64 = 0
while count < 3 {
    print(count)
    count = count + 1
}
```

Use `break` to leave the current loop and `continue` to skip to its next iteration. Labeled loops and labeled break/continue are not supported.

## For ranges and arrays

```nx
for i in 0..5 {
    print(i)
}

let values = [10, 20, 30]
for value in values {
    print(value)
}
```

The `..` range is half-open: the start is included and the end is excluded. The current parser does not implement the Rust-style `..=` range syntax or iterator methods such as `.enumerate()` / `.rev()`.

## Match statements

```nx
enum Color {
    Red,
    Green,
    Blue
}

fn main() {
    let color = Color::Green
    match color {
        Color::Red => { print("red") }
        Color::Green => { print("green") }
        _ => { print("other") }
    }
}
```

Match arms have expression patterns and statement blocks. Unit enum variants and the wildcard `_` are supported. Match is not a general expression in this compiler version; payload enum variants, match guards, destructured struct patterns, or-patterns and range patterns are not supported.

## Returns

Functions use an explicit `return` statement for a non-void return value:

```nx
fn max(a: i64, b: i64) -> i64 {
    if a > b {
        return a
    } else {
        return b
    }
}
```

The compiler checks that a non-void function returns a value on every path it can prove. It does not support Rust-style `loop { break value }` expressions, block expressions that return a value, or `?` error propagation in the current parser.
