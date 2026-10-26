# Standard Library Overview

The current checked-in standard library consists of small wrapper modules. Importing a module loads its functions into the program; use the names declared in the source files (for example, `fs_read`), rather than assuming methods such as `fs.read_to_string` exist.

## Current modules

| Module | API | Notes |
|---|---|---|
| [`std.io`](io.md) | `read_line`, `read_i64`, `read_f64` | Prompted console input |
| [`std.fs`](fs.md) | `fs_read`, `fs_read_checked`, `fs_write`, `fs_exists` | Whole-file string I/O, including unambiguous checked reads |
| [`std.collections`](collections.md) | `int_list_*`, `string_list_*` | Checked typed growable lists; not generic containers |
| [`std.net`](net.md) | `http_get_text` | One simple HTTP GET text helper |
| [`std.json`](json.md) | `json_field` | Simple scalar field extraction |
| [`std.db`](db.md) | `db_open`, `db_exec`, `db_close` | Open SQLite, execute SQL and close |
| [`std.memory`](memory.md) | `memory_alloc`, `memory_free` | Low-level allocation helpers |
| [`std.crypto`](crypto.md) | `hash_sha256` | SHA-256 hex digest for a string |
| [`std.os`](os.md) | `args_count`, `args_get` | Command-line arguments |
| [`std.string`](string.md) | `len`, `byte_at`, `byte_len` | UTF-8 byte length and byte access |
| [`std.vec2`](vec2.md) | `Vec2`, arithmetic, dot, length, normalize, lerp, move-towards | 2D vector values |
| [`std.vec3`](vec3.md) | `Vec3`, arithmetic, dot/cross, length, normalize, lerp | 3D vector values |
| [`std.ui`](ui.md) | UIRect, row/column/grid layout, panel, button, checkbox, slider, progress | Immediate-mode UI helper module |
| [`std.engine2d`](engine2d.md) | Window, frame loop, input, primitive drawing | Core 2D runtime wrappers |
| [`std.tilemap`](tilemap.md) | Bounded grid storage and access | Tile data for simple worlds |
| [`std.collision2d`](collision2d.md) | AABB/circle, tile movement, point and circle-box tests | 2D collision helpers |
| [`std.physics2d`](physics2d.md) | Gravity, jump launch, friction, velocity clamping and tilemap movement | Lightweight platformer math; not a rigid-body solver |
| [`std.animation2d`](animation2d.md) | Sprite frames, easing curves, ping-pong | Time-based animation helpers |
| [`std.sprite2d`](sprite2d.md) | Texture and sprite-sheet rendering | Sprite resources and drawing |
| [`std.camera2d`](camera2d.md) | Follow, transforms, bounds clamp and runtime camera | 2D camera helpers |

## Import and call a function

```nx
import std.io
import std.string

fn main() {
    let name = read_line("Name: ")
    print("Hello, " + name)
    print("Byte length: " + str_i64(len(name)))
}
```

Built-ins such as `print`, `input`, `input_i64`, `file_read`, `http_get` and `sqlite_open` are provided by the compiler/runtime. Standard-library files wrap some of these with a small named API.

## Limitations

The current library does not include the larger APIs previously described in some pages, such as a full JSON object model, recursive directory iterator, TCP/UDP socket types, SQL query-row abstraction, cryptographic RNG, or arena/pool allocators. The pages in this section document the implemented wrappers only; use the compiler's actual built-ins and runtime sources as the source of truth for additional functionality.
