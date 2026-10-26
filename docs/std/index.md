# Standard Library

The checked-in standard library is deliberately small. Each `import std.NAME` loads the corresponding file from the installed or portable `stdlib/` directory. The current standard modules are wrappers around built-in functions; they are not a Rust-style library of rich result types, iterators or filesystem objects.

## Modules

| Module | Functions currently provided |
|---|---|
| [Overview](overview.md) | Module map and limitations |
| [std.io](io.md) | `read_line`, `read_i64`, `read_f64` |
| [std.fs](fs.md) | `fs_read`, `fs_read_checked`, `fs_write`, `fs_exists` |
| [std.net](net.md) | `http_get_text` |
| [std.json](json.md) | `json_field` |
| [std.db](db.md) | `db_open`, `db_exec`, `db_close` |
| [std.memory](memory.md) | `memory_alloc`, `memory_free` |
| [std.crypto](crypto.md) | `hash_sha256` |
| [std.os](os.md) | `args_count`, `args_get` |
| [std.string](string.md) | `len`, `byte_at`, `byte_len` |
| [std.math](math.md) | Scalar math and trigonometry helpers |
| [std.vec2](vec2.md) | Vec2 construction and 2D vector math |
| [std.vec3](vec3.md) | Vec3 construction and 3D vector math |
| [std.collections](collections.md) | Checked integer and string growable lists |
| [std.ui](ui.md) | Immediate-mode widgets and rectangle layout helpers |
| [std.option](option.md) | Concrete `Option` enum with checked extraction and fallback helpers |
| [std.result](result.md) | Concrete `Result` enum with checked extraction and error-text retrieval |
| [std.engine2d](engine2d.md) | Core 2D window/frame/input/drawing helpers |
| [std.tilemap](tilemap.md) | Tilemap data storage and safe access |
| [std.collision2d](collision2d.md) | AABB, circle and tilemap collision movement |
| [std.physics2d](physics2d.md) | Gravity, jump velocity, friction and tilemap movement |
| [std.tilemap_render](tilemap_render.md) | Viewport tilemap rendering |
| [std.animation2d](animation2d.md) | Time-based sprite frame selection |
| [std.sprite2d](sprite2d.md) | Texture and sprite-sheet drawing |
| [std.camera2d](camera2d.md) | Camera wrappers |
| [std.audio2d](audio2d.md) | Optional audio wrappers |

## Combined example

```nx
import std.fs
import std.json
import std.io

fn main() {
    let path = "settings.json"
    if fs_exists(path) {
        let content = fs_read(path)
        let theme = json_field(content, "theme")
        print("Theme: " + theme)
    }

    let name = read_line("Your name: ")
    print("Hello, " + name)
}
```

The example expects `settings.json` to contain a simple top-level field such as `{"theme":"dark"}`. These functions return plain strings/status codes; they do not return `Result` or optional values.
