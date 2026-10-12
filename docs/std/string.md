# `std.string`

The current `stdlib/string.nx` module exposes byte-oriented helpers:

- `len(s: string) -> i64`
- `byte_len(s: string) -> i64`
- `byte_at(s: string, index: i64) -> i64`

Both length functions currently return the number of UTF-8 bytes, not Unicode code points or user-perceived characters. `byte_at` uses a zero-based index and returns the byte value (0–255) or -1 when the index is outside the string.

## Example

```nx
import std.string

fn main() {
    let text = "NEXUS"
    print("Byte length: " + str_i64(len(text)))
    print("First byte: " + str_i64(byte_at(text, 0)))
    print("Out of range: " + str_i64(byte_at(text, 99)))
}
```

For non-ASCII text, one displayed character may occupy multiple UTF-8 bytes. These wrappers do not provide normalization, grapheme iteration, regular expressions, splitting, trimming or a general-purpose string builder.
