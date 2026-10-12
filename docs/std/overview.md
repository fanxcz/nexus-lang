# Standard Library Overview

NEXUS provides a minimal but powerful standard library focused on systems programming, game development, and application development.

## Module Index

| Module | Description | Status |
|--------|-------------|--------|
| [`std.io`](io.md) | Input/output, printing, formatting | ✅ Stable |
| [`std.fs`](fs.md) | File system operations | ✅ Stable |
| [`std.net`](net.md) | Networking (HTTP client, sockets) | ✅ Stable |
| [`std.json`](json.md) | JSON parsing/serialization | ✅ Stable |
| [`std.db`](db.md) | SQLite database interface | ✅ Stable |
| [`std.memory`](memory.md) | Memory allocation, layout | ✅ Stable |
| [`std.crypto`](crypto.md) | Cryptographic primitives (SHA-256) | ✅ Stable |

## Common Patterns

### Importing

```nx
// Import entire module
import std.io
import std.fs

// Selective import
import std.json { parse, stringify, JsonValue }

// Alias
import std.net as net
```

### Error Handling

Most stdlib functions return `Result<T, E>`:

```nx
import std.fs
import std.io

fn read_config() -> Result<Config, Error> {
    let content = fs.read_to_string("config.json")?
    let config = std.json.parse(content)?
    Ok(config)
}
```

## std.io

Core I/O operations:

```nx
import std.io

fn main() {
    // Printing
    print("Hello")
    println("World")
    eprint("Error: ")
    eprintln("Failed!")

    // Formatting
    let name = "NEXUS"
    let version = 1.1
    println("{} v{}", name, version)
    println!("{name} v{version}")  // Named format

    // Reading input
    let line = io.read_line()?
    let num: i32 = io.parse_input()?
}
```

## std.fs

File system operations:

```nx
import std.fs

fn main() {
    // Read/write
    let content = fs.read_to_string("file.txt")?
    fs.write("output.txt", content)?

    // Path operations
    let path = fs.join("dir", "subdir", "file.txt")
    let exists = fs.exists(path)
    let is_dir = fs.is_dir(path)
    let metadata = fs.metadata(path)?

    // Directory iteration
    for entry in fs.read_dir(".") {
        println("{}", entry.name())
    }

    // Create/remove
    fs.create_dir_all("path/to/dir")?
    fs.remove("file.txt")?
    fs.remove_dir_all("old_dir")?
}
```

## std.net

Networking (HTTP client, TCP/UDP sockets):

```nx
import std.net

fn main() {
    // HTTP GET
    let response = net.get("https://api.example.com/data")?
    println("Status: {}", response.status)
    println("Body: {}", response.text()?)

    // HTTP POST with JSON
    let json = std.json.stringify({ "key": "value" })
    let resp = net.post("https://api.example.com/endpoint", json)?
    
    // TCP (planned)
    // let stream = net.tcp_connect("127.0.0.1:8080")?
}
```

## std.json

JSON parsing and serialization:

```nx
import std.json

fn main() {
    // Parse
    let json = parse(r#"{"name": "NEXUS", "version": 1.1, "tags": ["lang", "game"]}"#)?
    
    // Access fields
    let name = json.get_str("name")?
    let version = json.get_f64("version")?
    let tags = json.get_array("tags")?
    
    // Serialize
    let data = JsonValue::Object([
        ("name", JsonValue::String("NEXUS")),
        ("version", JsonValue::Number(1.1)),
    ])
    let output = stringify(data)
    println(output)
}
```

## std.db

SQLite database interface:

```nx
import std.db

fn main() {
    let conn = db.open("game.db")?
    defer db.close(conn)
    
    // Execute
    db.execute(conn, "CREATE TABLE IF NOT EXISTS players (id INTEGER PRIMARY KEY, name TEXT, score INTEGER)")?
    
    // Query
    let rows = db.query(conn, "SELECT name, score FROM players WHERE score > ?", [1000])?
    for row in rows {
        let name = row.get_str(0)?
        let score = row.get_i64(1)?
        println("{}: {}", name, score)
    }
    
    // Prepared statements
    let stmt = db.prepare(conn, "INSERT INTO players (name, score) VALUES (?, ?)")?
    db.execute_stmt(stmt, ["Player1", 1500])?
}
```

## std.memory

Low-level memory operations:

```nx
import std.memory

fn main() {
    // Allocation
    let ptr = mem_alloc(1024)  // *u8
    defer mem_free(ptr)
    
    // Zeroed allocation
    let buf = mem_alloc_zero(512)
    
    // Reallocation
    let new_ptr = mem_realloc(ptr, 2048)
    
    // Memory operations
    mem_copy(dst, src, 100)
    mem_set(ptr, 0, 1024)
    
    // Layout
    let layout = Layout::new::<Vec2>()
    let aligned_ptr = mem_alloc_aligned(layout.size, layout.align)
}
```

## std.crypto

Cryptographic primitives:

```nx
import std.crypto

fn main() {
    // SHA-256
    let data = b"Hello, NEXUS!"
    let hash = crypto.sha256(data)
    println("SHA-256: {}", hash.to_hex())
    
    // Random bytes (planned)
    // let rand = crypto.random_bytes(32)
}
```

## Using with Build System

Standard library modules are automatically available. No extra configuration needed in `nexus.toml`.

```toml
# nexus.toml - stdlib is implicit
[package]
name = "my-app"
version = "1.0.0"
edition = "2026"

[build]
entry = "src/main.nx"
```