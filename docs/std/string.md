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

## Built-in string functions

The runtime additionally provides these built-ins directly (no import needed):

- `string.contains(s, sub) -> bool` — substring test
- `string.find(s, sub) -> i64` — byte index of the first occurrence, or -1
- `string.trim(s) -> string` — copy without leading/trailing ASCII whitespace
- `string.upper(s) / string.lower(s) -> string` — ASCII case conversion
- `string.slice(s, start, end) -> string` — substring by byte range, boundaries clamped
- `string.split_count(s, sep) -> i64` — number of separator-separated fields (at least 1)
- `string.split_at(s, sep, index) -> string` — field `index` (0-based) after splitting by `sep`; empty string when out of range

Because the language has no string arrays yet, splitting is exposed through the `split_count` + `split_at` pair. All indexes and lengths are UTF-8 byte offsets.

```nx
fn main() {
    let csv = "firefox.exe,384,Console"
    let n = string.split_count(csv, ",")
    print(string.split_at(csv, ",", 0))   // firefox.exe
    print(string.split_count(csv, ","))   // 3
    print(string.upper("nexus"))          // NEXUS
    print(string.find(csv, "Console"))    // 13
}
```
