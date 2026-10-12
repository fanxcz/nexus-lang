# `std.fs`

The current `stdlib/fs.nx` module wraps simple whole-file operations.

## Functions

- `fs_read(path: string) -> string`
- `fs_write(path: string, data: string) -> i64`
- `fs_exists(path: string) -> bool`

`fs_read` returns an empty string when it cannot read a file, which is indistinguishable from a successfully read empty file. Check `fs_exists` first when that difference matters. `fs_write` returns 0 on success and a negative value on failure.

## Example

```nx
import std.fs

fn main() {
    let path = "notes.txt"
    if fs_exists(path) {
        let content = fs_read(path)
        print("Existing content: " + content)
    } else {
        print("File does not exist yet")
    }

    let rc = fs_write(path, "Hello from NEXUS")
    if rc == 0 {
        print("Saved " + path)
    } else {
        print("Could not write " + path)
    }
}
```

This wrapper operates on whole files and strings. It does not currently provide directory creation/listing, file metadata, byte buffers, rename/copy helpers, or path manipulation objects.
