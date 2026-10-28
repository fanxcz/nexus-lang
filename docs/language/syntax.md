# NEXUS Language Syntax (1.1)

This page documents syntax supported by the current compiler. Types are inferred from initializers where possible; explicit types can be used in parameters, return values and declarations.

## Variables and constants

```nx
let value = 42
let name = "NEXUS"
let mut counter: i64 = 0
const limit: i64 = 100
counter = counter + 1
```

Bindings declared with `let` are immutable unless `mut` is specified.

### Discard binding `_`

`_` is a special name for "value not needed": every `let _ = ...` creates a
fresh hidden slot, so it can be repeated any number of times in the same scope
and `_` cannot be referenced afterwards.

```nx
let _ = system("echo hi >nul")
let _ = system("echo bye >nul")   // no "duplicate variable" error
```

### String concatenation with non-strings

When one side of `+` is a string, the other side may be `i64`, `f64` or `bool`;
the compiler wraps it in `str_i64` / `str_f64` / `str_bool` automatically:

```nx
print("code=" + 42)      // code=42
print("ok=" + true)      // ok=true
```


### String escapes and Unicode

String literals support the common escapes `\n`, `\t`, `\r`, `\\`, and `\"`. Use `\u{HEX}` to include a Unicode scalar as UTF-8:

```nx
fn main() {
    print("Copyright \u{00A9}")
    print("Hello \u{1F642}")
}
```

The Unicode escape requires 1–6 hexadecimal digits inside braces. Values above `U+10FFFF`, UTF-16 surrogate values, and `U+0000` are rejected with a diagnostic. The string ABI uses null-terminated strings, so embedded NUL is not currently supported.

## Functions

```nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}

fn main() {
    print(add(40, 2))
}
```

The entry point of a native program is `fn main()`.

## Conditions and loops

```nx
if value > 10 {
    print(value)
} else if value == 10 {
    print("ten")
} else {
    print(0)
}

let mut index: i64 = 0
while index < 3 {
    print(index)
    index += 1
}

for item in 1..4 {
    print(item)
}
```

Loop control supports `break` and `continue`.

### Deferred cleanup

Use `defer` to schedule a call that returns `void` when the current block is
left. Arguments are evaluated when the `defer` statement is reached, and calls
run in last-in-first-out order. Cleanup runs on normal block exit, `return`,
`break`, and `continue`:

```nx
fn log(message: string) {
    print(message)
}

fn main() {
    let mut state = "before"
    defer log(state)
    state = "after"
    print("body")
}
```

This prints `body` followed by `before`: the deferred call captures the
argument value at registration time. The current implementation supports
deferred function calls only, and the LLVM backend is required; the built-in
x86-64 `--backend native` subset reports an explicit unsupported-feature
diagnostic rather than silently ignoring cleanup. `defer` does not intercept
process aborts or add exception handling.

You can also iterate over a string. The loop variable is an `i64` Unicode code point, not a UTF-8 byte. Convert a code point back to a one-character UTF-8 string with `string.from_char`:

```nx
let text = "A\u{00E9}\u{1F642}"
for codepoint in text {
    print(string.from_char(codepoint))
}
```

The string expression is evaluated once before iteration. Empty strings run zero iterations; a multibyte UTF-8 character still produces one loop iteration.

## Arrays, structs, enums and match

```nx
let mut values = [10, 20, 30]
values[1] = 42

struct Player {
    health: i64
    speed: f64
}

enum Color { Red, Green, Blue }

fn main() {
    let player = Player { health: 100, speed: 3.5 }
    let selected = Color::Green
    match selected {
        Color::Red => { print("red") }
        Color::Green => { print("green") }
        _ => { print("other") }
    }
    print(player.health)
}
```

Fixed-size arrays can be declared explicitly as `[i64; 3]`. Runtime checks protect variable-index array access.

## Integer bitwise and shift operators

The `i64` type supports `&`, `|`, `^`, `<<`, `>>` and their compound
assignment forms. Right shift is arithmetic (the sign bit is preserved).
For both shift directions, only the low six bits of the shift count are used;
this is consistent between the built-in x86-64 backend and LLVM. For example,
`1 << 64` behaves like `1 << 0`, and `-8 >> 65` behaves like `-8 >> 1`.
This rule applies to compound forms such as `value <<= count` too.

```nx
let mask = 1 << 4
let signed = -8 >> 1
let mut flags = 3
flags <<= 1
flags |= mask
print(signed)
print(flags)
```

## Imports

```nx
import std.io
import std.fs
import std.json
```

Standard modules resolve from the installed or portable `stdlib/` directory. Project-relative imports are resolved relative to the importing source file and supported project/vendor paths.

## Input, files and environment

```nx
let name = input("Name: ")
let age = input_i64("Age: ")
print("Hello, " + name)
print(str_i64(age + 1))

file_write("save.txt", "hello")
let data = file_read("save.txt")
let exists = file_exists("save.txt")
let home = env("HOME")
```

## Terminal input loop

This example uses the console input/output built into the runtime:

```nx
fn main() {
    screen_begin()
    let mut running = true
    while running {
        screen_clear()
        screen_put(10, 5, "@")
        screen_present()
        if key_pressed() {
            let key = read_key()
            if key == 113 { running = false }
        }
        sleep(30)
    }
    screen_end()
}
```

NEXUS also provides SDL2-backed graphics and audio APIs for native applications and games; this example is specifically a terminal interface.

## Project templates

```sh
nexus new mycli --template cli
nexus new myapp --template app
nexus new mygame --template game
nexus new MyPaint --template paint
```

The starter source files live in `examples/templates/` in the source repository. `nexus new` copies the selected template into the new project's `src/main.nx`; the public CLI commands and `.nx` extension remain unchanged.

## Current language-feature status

The implemented language includes functions, variables, primitive types, arrays, structs, unit enums, imports, conditionals, loops, match, pointers, and external functions. Full generic monomorphization, trait dispatch, closure/async lowering, and a full JSON AST remain experimental or unimplemented; do not rely on them as stable language features.

### Growable lists

Fixed arrays require a size up front. For open-ended accumulation the runtime
provides growable lists behind opaque handles — `ilist_*` for `i64` values and
`slist_*` for strings. Values live for the whole process; out-of-range `get`
returns `0` / `""` instead of crashing.

```nx
let squares = ilist_new()
let mut i = 0
while i < 10 {
    ilist_push(squares, i * i)
    i = i + 1
}
print(ilist_len(squares))   // 10
print(ilist_get(squares, 4)) // 16

let words = slist_new()
slist_push(words, "hello")
print(slist_get(words, 0))   // hello
```
