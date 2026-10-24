# `std.os`

The current `stdlib/os.nx` module wraps two command-line argument helpers:

- `args_count() -> i64` returns the number of command-line arguments.
- `args_get(index: i64) -> string` returns an argument by zero-based index; a negative or out-of-range index returns an empty string.

The executable path is **not** included. If the program is run as `./tool alpha "beta gamma"`, argument 0 is `alpha`, argument 1 is `beta gamma`, and `args_count()` is 2.

## Example

```nx
import std.os

fn main() {
    let count = args_count()
    print("Arguments: " + str_i64(count))

    let mut i: i64 = 0
    while i < count {
        print(str_i64(i) + ": " + args_get(i))
        i = i + 1
    }
}
```

The wrappers do not currently expose environment enumeration, process spawning, current-directory changes or platform-specific OS handles. Use the built-in `env(name)` to read one environment variable. Platform argument handling is covered by the runtime CTest suite.

## Capturing command output

The built-in `capture_output(command: string) -> string` runs a shell command and returns everything it writes to stdout as a string (empty string on failure). It complements `system()`, which only returns the exit code:

```nx
fn main() {
    let listing = capture_output("tasklist /FO CSV /NH")
    let lines = string.split_count(listing, "\n")
    print("process entries: " + lines)
}
```

`capture_output` requires the LLVM backend (`--backend llvm`) and is not part of the built-in native subset.
