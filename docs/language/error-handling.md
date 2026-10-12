# Error Handling

NEXUS uses **Result/Option types** for error handling - no exceptions.

## Option Type

```nx
enum Option<T> {
    Some(T),
    None,
}

// Usage
let opt: Option<i32> = Some(42)
let empty: Option<i32> = None

// Pattern matching
match opt {
    Some(v) => print("Value: {v}"),
    None => print("No value"),
}

// Helper methods
opt.is_some()      // bool
opt.is_none()      // bool
opt.unwrap()       // T (panics if None)
opt.unwrap_or(0)   // T (default if None)
opt.expect("msg")  // T (panic with message if None)
```

## Result Type

```nx
enum Result<T, E> {
    Ok(T),
    Err(E),
}

// Usage
fn divide(a: f64, b: f64) -> Result<f64, str> {
    if b == 0.0 { Err("Division by zero") }
    else { Ok(a / b) }
}

match divide(10.0, 2.0) {
    Ok(result) => print("Result: {result}"),
    Err(e) => print("Error: {e}"),
}
```

## The `?` Operator

```nx
fn read_config() -> Result<Config, Error> {
    let content = fs_read_to_string("config.json")?  // Returns Err on failure
    let config = json_parse(content)?                 // Returns Err on failure
    Ok(config)
}

// Equivalent to:
fn read_config() -> Result<Config, Error> {
    let content = match fs_read_to_string("config.json") {
        Ok(c) => c,
        Err(e) => return Err(e),
    };
    let config = match json_parse(content) {
        Ok(c) => c,
        Err(e) => return Err(e),
    };
    Ok(config)
}
```

## Error Types

```nx
// String errors (simple)
type StrError = str

// Structured errors
enum IoError {
    NotFound(str),
    PermissionDenied(str),
    InvalidData(str),
    Other(str),
}

enum ParseError {
    InvalidFormat(str),
    UnexpectedEof,
    NumberTooLarge,
}
```

## Result Helpers

```nx
let result: Result<i32, str> = Ok(42)

// Map success value
result.map(|x| x * 2)           // Ok(84)

// Map error
result.map_err(|e| e.to_uppercase())

// Chain operations
result.and_then(|x| Ok(x + 1))  // Ok(43)

// Unwrap with default
result.unwrap_or(0)             // 42
result.unwrap_or_else(|| 0)     // 42 (lazy)

// Convert to Option
result.ok()                     // Some(42)
result.err()                    // None
```

## Panic vs Result

| Situation | Use |
|-----------|-----|
| Programming bug (invariant violation) | `panic!` / `unreachable!` |
| Expected runtime failure | `Result` |
| Optional value | `Option` |
| FFI boundary | `Result` or sentinel values |

```nx
// Panic - bug in code
fn get_first(items: []i32) -> i32 {
    if items.is_empty() { panic!("empty slice") }
    items[0]
}

// Result - expected failure
fn try_get_first(items: []i32) -> Option<i32> {
    if items.is_empty() { None }
    else { Some(items[0]) }
}
```

## Custom Error Types

```nx
enum GameError {
    InvalidMove(str),
    NotYourTurn,
    GameOver,
    Io(IoError),  // Wrap other errors
}

fn make_move(game: &mut Game, mv: Move) -> Result<(), GameError> {
    if game.is_over() { return Err(GameError::GameOver) }
    if !game.is_valid(mv) { return Err(GameError::InvalidMove("...")) }
    // ...
    Ok(())
}
```

## Error Stack Traces (Debug)

```nx
// In debug builds, errors capture stack trace
let result = risky_operation()
if let Err(e) = result {
    eprint("Error: {e}")
    eprint("Stack: {e.stack_trace()}")  // Debug only
}
```

## Best Practices

1. **Use `Result` for fallible operations** - I/O, parsing, validation
2. **Use `Option` for nullable values** - lookup, optional config
3. **Propagate with `?`** - clean, readable error chains
4. **Define domain-specific errors** - `GameError`, `NetworkError`
5. **Don't use panic for control flow** - only for bugs
6. **Document error variants** - what each means and when it occurs