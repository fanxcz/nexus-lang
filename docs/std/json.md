# `std.json`

The current `stdlib/json.nx` module provides one helper:

```nx
json_field(json: string, key: string) -> string
```

It wraps the built-in `json_get(json, key)` runtime function. This is a lightweight text extractor, **not a complete JSON parser**. It searches for the requested quoted key, looks for the following colon, and returns a string value or raw scalar text. It does not implement nested paths, arrays/objects as structured values, schema validation, conversions or serialization.

## Example

```nx
import std.json

fn main() {
    let data = "{\"theme\":\"dark\",\"version\":1}"
    let theme = json_field(data, "theme")
    let version = json_field(data, "version")
    print("Theme: " + theme)
    print("Version: " + version)
}
```

Use this helper only for simple, trusted JSON strings where the requested key is unambiguous. It searches text rather than parsing object structure, so duplicate names or the same key text in nested data can produce incorrect results. An empty string also means the key was not found or could not be extracted.
