# Standard Library

Core modules for systems programming and application development.

## In This Section

| Module | Description |
|--------|-------------|
| [Overview](overview.md) | Standard library overview |
| [std.io](io.md) | Input/output operations |
| [std.fs](fs.md) | File system operations |
| [std.net](net.md) | Networking (HTTP, sockets) |
| [std.json](json.md) | JSON parsing/serialization |
| [std.db](db.md) | SQLite database interface |
| [std.memory](memory.md) | Memory allocation |
| [std.crypto](crypto.md) | Cryptographic primitives |

## Quick Example

```nx
import std.io
import std.fs
import std.json

fn main() {
    // Read config file
    let content = fs.read_to_string("config.json")?
    
    // Parse JSON
    let config = json.parse(content)?
    let port = config.get_u16("port")?
    
    println("Server starting on port {}", port)
}
```