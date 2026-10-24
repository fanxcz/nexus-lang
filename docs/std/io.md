# `std.io`

The current `stdlib/io.nx` file provides three prompted input helpers. Printing is available through the language's built-in `print` function.

## Functions

- `read_line(prompt: string) -> string`
- `read_i64(prompt: string) -> i64`
- `read_f64(prompt: string) -> f64`

## Example

```nx
import std.io

fn main() {
    let name = read_line("Name: ")
    print("Hello, " + name)

    let age = read_i64("Age: ")
    print("Next year: " + str_i64(age + 1))

    let ratio = read_f64("Ratio: ")
    print("Ratio: " + str_f64(ratio))
}
```

These helpers return plain values, not a `Result`; do not expect structured parse errors or a retry policy from the wrapper. The module does not currently expose `println`, `eprint`, stream objects, formatting macros or `flush` wrappers.
