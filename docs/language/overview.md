# Language Overview

NEXUS is an actively developed, statically checked compiled language. This repository contains a C++20 compiler, a built-in x86-64 machine-code backend for a defined subset, an LLVM/Clang compilation path, a small standard library and a native runtime.

## First program

```nx
fn main() {
    print("Hello from NEXUS!")
}
```

Save it as `hello.nx`, then run:

```bash
nexus check hello.nx
nexus run hello.nx
```

## Supported language basics

The current compiler implements functions, typed parameters and returns, inferred bindings, mutable bindings, primitive types, fixed arrays, structs, unit and single-scalar-payload enums, file imports, conditionals, numeric/array/Unicode-string loops, `match` statements, pointers, and limited external function declarations. Backend support differs for some language features.

```nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}

fn main() {
    let mut total: i64 = 0
    for i in 0..3 {
        total = total + add(i, 10)
    }
    print(total)
}
```

## Backends

The built-in machine-code backend emits Windows PE or Linux ELF executables for a documented x86-64 subset without invoking a separately installed compiler. Programs outside the subset may use the LLVM/Clang path and its toolchain. A target listed by the CLI is not necessarily supported by every backend. Use `nexus explain-backend file.nx` to see the selection for one file.

## Current limitations

Generic types, traits, closures/async, and Rust-style `impl` blocks are not implemented. Concrete enum payloads support one payload of `i64`, `f64`, `bool`, `string`, or pointer type; generic/nested payloads are not supported. The `?` operator works for matching `Some(T)/None` and `Ok(T)/Err` enums on the LLVM backend when the enclosing function returns the same enum type. `defer` supports void-returning calls on the LLVM backend, while the built-in native backend reports it as unsupported. Generic `Option<T>` / `Result<T, E>`, cross-enum propagation, a full JSON object model and complete FFI support remain unimplemented. See [Syntax](syntax.md), [Types](types.md), and [Error Handling](error-handling.md) for exact boundaries.

Read [Quick Start](../getting-started/quickstart.md), the [Language Syntax](syntax.md), [Standard Library](../std/index.md) and [Runtime API](../runtime/index.md) to get started.
