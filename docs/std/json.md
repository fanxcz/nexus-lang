# std.json

JSON parsing and serialization.

## Types

```nx
enum JsonValue {
    Null,
    Bool(bool),
    Number(f64),      // All numbers as f64
    String(str),
    Array([]JsonValue),
    Object(Map<str, JsonValue>),
}
```

## Parsing

```nx
import std.json

// Parse from string
let json = json.parse(r#"{"name": "NEXUS", "version": 1.1}"#)?
let arr = json.parse("[1, 2, 3]")?

// Parse from file
let content = fs.read_to_string("config.json")?
let config = json.parse(content)?
```

## Accessing Values

```nx
let json = json.parse(r#"{"user": {"name": "Alice", "age": 30}, "tags": ["dev", "lang"]}"#)?

// Get by key (returns Result)
let name = json.get_str("user.name")?        // "Alice"
let age = json.get_f64("user.age")?          // 30.0
let tags = json.get_array("tags")?            // []JsonValue

// Direct access (panics if missing/wrong type)
let name = json["user"]["name"].as_str()

// Check existence
let has_email = json.has_key("email")

// Iterate object
for (key, value) in json.as_object() {
    println("{key}: {value}")
}

// Iterate array
for item in json.as_array() {
    println("{}", item)
}
```

## Type-Safe Access

```nx
let json = json.parse(r#"{"count": 42, "rate": 3.14, "active": true}"#)?

// Numbers
let count: i32 = json.get_i32("count")?      // 42
let count: u64 = json.get_u64("count")?      // 42
let rate: f32 = json.get_f32("rate")?        // 3.14
let rate: f64 = json.get_f64("rate")?        // 3.14

// Booleans
let active = json.get_bool("active")?        // true

// Strings
let name = json.get_str("name")?             // "NEXUS"

// Arrays
let nums = json.get_array("numbers")?        // []JsonValue
let ints = json.get_i32_array("numbers")?    // []i32 (if all integers)
```

## Serialization

```nx
// Build manually
let data = JsonValue::Object([
    ("name", JsonValue::String("NEXUS")),
    ("version", JsonValue::Number(1.1)),
    ("tags", JsonValue::Array([
        JsonValue::String("lang"),
        JsonValue::String("game"),
    ])),
    ("active", JsonValue::Bool(true)),
])

// Serialize to string
let output = json.stringify(data)
// {"name":"NEXUS","version":1.1,"tags":["lang","game"],"active":true}

// Pretty print
let pretty = json.stringify_pretty(data, 2)
// {
//   "name": "NEXUS",
//   "version": 1.1,
//   "tags": ["lang", "game"],
//   "active": true
// }
```

## Builder Pattern

```nx
let data = json.object()
    .set("name", "NEXUS")
    .set("version", 1.1)
    .set("tags", json.array(["lang", "game"]))
    .set("config", json.object()
        .set("debug", false)
        .set("max_fps", 60)
    )
    .build()

let output = json.stringify(data)
```

## Helpers

```nx
// Merge objects
let merged = json.merge(obj1, obj2)

// Deep clone
let copy = json.clone(json_value)

// Equality
let equal = json.equal(json1, json2)

// Minify (remove whitespace)
let minified = json.minify(pretty_json)
```

## Example

```nx
import std.json
import std.fs

fn main() {
    // Load config
    let content = fs.read_to_string("config.json")?
    let config = json.parse(content)?
    
    // Read settings
    let port = config.get_u16("server.port")?
    let host = config.get_str("server.host")?
    let debug = config.get_bool("debug")?
    
    // Modify
    config.set("last_run", json.current_timestamp())
    
    // Save back
    let updated = json.stringify_pretty(config, 2)
    fs.write("config.json", updated)?
}
```