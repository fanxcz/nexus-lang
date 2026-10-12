# std.io

Input/output operations.

## Functions

```nx
import std.io

// Print to stdout (no newline)
print("Hello")
print("Value: {x}")

// Print to stdout with newline
println("Hello")
println("Value: {x}")

// Print to stderr
eprint("Error: ")
eprintln("Failed!")

// Formatted printing
println("{} {}", "hello", 42)
println("{name} is {age}", name="Alice", age=30)

// Read line from stdin
let line = io.read_line()?

// Parse input as type
let num: i32 = io.parse_input()?

// Flush stdout
io.flush()
```

## Types

```nx
// Standard streams
io.stdout()  // *File
io.stderr()  // *File
io.stdin()   // *File
```

## Example

```nx
import std.io

fn main() {
    println("Enter your name:")
    let name = io.read_line()?
    println("Hello, {name}!")
    
    println("Enter a number:")
    let num: i32 = io.parse_input()?
    println("You entered: {num}")
}
```