# Error Handling (Current Scope)

The current compiler does not implement `Result<T, E>`, `Option<T>`, the `?` propagation operator, `panic!`, `unwrap` helpers or custom error-enum handling. Older versions of this page described planned features as if they were implemented; those examples will not pass `nexus check`.

## Check sentinel return values

Most low-level runtime wrappers return a plain value. Each wrapper defines its own failure convention, so inspect the relevant API instead of expecting a universal error type.

```nx
fn main() {
    let text = file_read("settings.json")
    if text == "" {
        print("The file is missing, unreadable, or empty")
        return
    }

    let rc = file_write("copy.txt", text)
    if rc != 0 {
        print("Could not write the file; code: " + str_i64(rc))
        return
    }

    print("File copied")
}
```

An empty string from `file_read` is ambiguous: it can mean an error or a legitimately empty file. `file_exists(path)` can help in simple cases, but it does not eliminate every possible I/O race or permission error.

## Check graphics, files, HTTP and SQLite

| API | Typical failure signal |
|---|---|
| `gfx_init(width, height, title)` | Returns `false`; retrieve the diagnostic with `gfx_error()` |
| `gfx_texture_load(path)` / `gfx_canvas_load_bmp(path)` | Returns ID 0 on failure |
| `file_read(path)` / `fs_read(path)` | Empty string on failure (also possible for an empty file) |
| `file_write(path, text)` / `fs_write(path, text)` | Integer status; 0 means success for these wrappers |
| `http_get(url)` / `http_get_text(url)` | Empty string when the request fails or the response body is empty |
| `db_open(path)` | Handle 0 on failure |
| `db_exec(handle, sql)` | SQLite result code; 0 means success |

## Use explicit control flow

Use `if`, early `return`, and meaningful error messages to handle failure. The compiler does not have exception-catch syntax, `defer`, `Result` / `Option`, or error propagation with `?`.

If an API is important to your application, write a narrow wrapper that checks its documented sentinel value and returns a value/status that your own program consistently interprets. Be careful not to invent a rich error guarantee around a runtime function that only returns a string or integer.
