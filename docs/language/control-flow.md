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

The `..` range is half-open: the start is included and the end is excluded. Both
bounds are evaluated once, before the loop starts, and must be `i64`. The loop
variable is scoped to the loop body and is immutable.

A `for` loop can also iterate over a fixed-size array:

```nx
let values = [10, 20, 30]
for value in values {
    print(value)
}
```

The array is evaluated once and each element is copied into the loop variable.
String iteration is not supported; use a numeric range with `string.len` and
`string.byte_at` if byte-level access is needed. The current parser does not
implement the Rust-style `..=` range syntax or iterator methods such as
`.enumerate()` / `.rev()`.

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

Match arms have patterns, optional guards, and statement blocks. Unit enum
variants, integer and boolean literals, inclusive integer ranges, binding
patterns, the wildcard `_`, and `if` guards are supported:

```nx
match score {
    100 => { print("perfect") }
    value if value > 90 => { print("great") }
    1..89 => { print("scored") }
    _ => { print("out of range") }
}
```

Match is not a general expression in this compiler version; payload enum
variants, destructured struct patterns, and or-patterns are not supported.
`match` does not perform exhaustiveness checking: if no arm matches, execution
continues after the `match` statement.

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
