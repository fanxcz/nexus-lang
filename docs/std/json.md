# `std.json`

NEXUS includes a bounded JSON parser in its runtime. The parser accepts standard JSON values and validates syntax, string escapes, UTF-8, number grammar and matching object/array delimiters. For safety, JSON input is limited to 16 MiB, nesting is limited to 128 levels, and the parser caps parsed values at one million nodes.

## API

| NEXUS helper | Result |
|---|---|
| `json_is_valid(json)` | `true` when the input is exactly one valid JSON value (object, array, string, number, boolean or `null`) |
| `json_field(json, key)` | Top-level object property as a decoded string, raw scalar text, or raw JSON object/array |
| `json_value_type(json, key)` | `string`, `number`, `boolean`, `null`, `object`, `array`, `missing` or `invalid` |
| `json_array_length(json, key)` | Length of a top-level array property, or `-1` if input/key/type is invalid |
| `json_array_item(json, key, index)` | Array element as a decoded string, raw scalar text, or raw JSON object/array |

All helpers are also available under the runtime names `json_valid`, `json_get`, `json_type`, `json_array_len` and `json_array_get`.

`json_field` only looks at properties of the root object; it will not accidentally match key text inside a string or inside a nested object. For duplicate property names, the last occurrence is used. Object/array results preserve the corresponding JSON source text instead of being re-serialized. A missing property returns an empty string; because a real JSON string may also be empty, call `json_value_type` to distinguish a missing key from an empty-string value.

The module currently does not provide schema validation, typed numeric conversions, object mutation or JSON serialization. Array access is by a root-level property name and integer index; arbitrary JSONPath expressions are not implemented.

## Example

```nx
import std.json

fn main() {
    let data = "{\"theme\":\"dark\",\"version\":1,\"items\":[\"a\",\"b\"]}"
    if !json_is_valid(data) {
        print("Invalid JSON")
        return
    }

    print(json_field(data, "theme"))
    print(json_field(data, "version"))
    print(json_value_type(data, "items"))
    print(json_array_length(data, "items"))
    print(json_array_item(data, "items", 1))
}
```
