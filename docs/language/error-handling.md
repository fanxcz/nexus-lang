# Error Handling (Current Scope)

NEXUS supports payload-bearing enum variants, concrete `Option`/`Result` enum patterns, and the `?` propagation operator on the LLVM backend. Generic `Option<T>` and `Result<T, E>` types, exception handling, and implicit conversion between different error enums are not implemented. The built-in native backend supports only a smaller subset and reports unsupported features explicitly.

## Check sentinel return values

Most low-level runtime wrappers return a plain value. Each wrapper defines its own failure convention, so inspect the relevant API instead of expecting a universal error type.

```nx
fn main() {
    let content: string = ""
    if !file_read_checked("settings.json", &content) {
        print("Could not read settings.json")
        return
    }

    let rc = file_write("copy.txt", content)
    if rc != 0 {
        print("Could not write the file; code: " + str_i64(rc))
        return
    }
    print("File copied")
}
```

An empty string from `file_read` is ambiguous: it can mean an error or a legitimately empty file. For a checked read, `file_read_checked(path, &out) -> bool` (or `std.fs`'s `fs_read_checked`) returns true for a successfully read empty file and false when opening, seeking, reading, closing, or allocating the output fails.

## Concrete enum payloads

Enum variants can carry one supported scalar payload:

```nx
enum ParseResult {
    Ok(i64),
    Err(string)
}

fn parse_example() -> ParseResult {
    return ParseResult::Err("invalid input")
}

fn main() {
    let result = parse_example()
    match result {
        ParseResult::Ok(value) => { print(value) }
        ParseResult::Err(message) => { print(message) }
    }
}
```

The current runtime representation supports `i64`, `f64`, `bool`, `string`, and pointer payloads. Struct, array, nested enum, and void payloads are rejected. This is not a generic algebraic data type implementation; use concrete enum declarations with a single supported payload type.

## Propagate an enum with `?`

The LLVM backend supports `?` when the expression has an enum with a matching `Some(T)/None` or `Ok(T)/Err` pair, and the enclosing function returns that same enum type:

```nx
enum ReadResult {
    Err(string),
    Ok(i64)
}

fn add_ten(result: ReadResult) -> ReadResult {
    let value = result?
    return ReadResult::Ok(value + 10)
}
```

The success variant's position in the enum does not affect propagation. A failure returns the original enum immediately; a success unwraps the payload. The enclosing function must return the same enum type. Cross-type propagation (for example, using `Option` inside a function returning `Result`) and generic `Result<T, E>` remain unsupported.

## Cleanup with `defer`

`defer` schedules a call returning `void` for lexical cleanup:

```nx
fn report(message: string) {
    print(message)
}

fn work() -> i64 {
    defer report("work finished")
    return 42
}
```

Arguments are evaluated immediately and deferred calls run in reverse order of registration. The LLVM backend runs them on normal block exit, `return`, `break`, and `continue`; the built-in native backend currently rejects `defer` with an explicit diagnostic. A runtime abort does not guarantee cleanup.

## Runtime failure signals

| API | Typical failure signal |
|---|---|
| `gfx_init(width, height, title)` | Returns false; inspect `gfx_error()` |
| `gfx_texture_load(path)` / `gfx_canvas_load_bmp(path)` | Returns ID 0 |
| `file_read(path)` / `fs_read(path)` | Empty string, which may also be a valid empty file |
| `file_write(path, text)` / `fs_write(path, text)` | Integer status; 0 means success for these wrappers |
| `http_get(url)` / `http_get_text(url)` | Empty string on failure or an empty response |
| `db_open(path)` | Handle 0 |
| `db_exec(handle, sql)` | SQLite result code; 0 means success |

NEXUS does not currently have exception-catch syntax. Handle each operation according to its documented status or sentinel value, and do not assume an API provides richer errors than it actually returns.
