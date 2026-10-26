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

The current compiler implements functions, typed parameters and returns, inferred bindings, mutable bindings, primitive types, fixed arrays, structs, unit enums, file imports, conditionals, numeric/array/Unicode-string loops, `match` statements, pointers and external function declarations in a limited form.

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

Generics, traits, closure/async lowering, Rust-style `impl` blocks, `defer`, `Result` / `Option`, `?` error propagation, a full JSON object model and complete FFI support are not implemented by the current parser. Some older documentation drafts described planned features as available; the current [Syntax](syntax.md), [Types](types.md) and [Functions](functions.md) pages separate implemented syntax from future work.

Read [Quick Start](../getting-started/quickstart.md), the [Language Syntax](syntax.md), [Standard Library](../std/index.md) and [Runtime API](../runtime/index.md) to get started.
