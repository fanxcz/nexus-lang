# Imports and Source Modules

The current implementation uses imports to combine source files. It does not implement Rust-style `module` declarations, `export`, selective imports, aliases, wildcard imports or directory-based `mod.nx` re-exports.

## Import standard-library helpers

A dotted import such as `import std.io` loads the source file `stdlib/io.nx` and adds the functions defined there to the current program:

```nx
import std.io
import std.fs

fn main() {
    let name = read_line("Name: ")
    fs_write("name.txt", name)
    print("Saved name")
}
```

Use the names declared by the imported module—for example `read_line`, `fs_read`, `fs_write`. These wrappers are plain functions, not namespaces that require calls like `fs.read_to_string(...)`.

## Import a project file

Imports can use a source path string relative to the importing file:

```nx
import "utility.nx"

fn main() {
    print(double(21))
}
```

The repository's `examples/imports.nx` demonstrates this file-based form. The compiler resolves a local source file and combines its functions, structs and enums with the importing source. Standard imports beginning with `std.` are resolved against the installed/portable `stdlib/` directory.

## Module-resolution notes

- Use a file path in a quoted import for local files, for example `import "modules/math.nx"`.
- Use `import std.NAME` for an actual file in the standard library, such as `stdlib/fs.nx` or `stdlib/crypto.nx`.
- Imported source is compiled as part of the program; there is no separate compilation unit/visibility layer.
- Circular imports are rejected with a readable chain diagnostic; acyclic imports are loaded once per compilation. This remains source-file composition, not Rust/Cargo module semantics.
- Package commands support local vendoring and archive creation; they do not provide a hosted registry with semver dependency resolution.

## What is not implemented

These patterns from older drafts are not supported by the current parser: `module math { ... }`, `export fn ...`, `import math as m`, `import math { add }`, `import math::*`, directory re-exports via `mod.nx`, and conditional modules via `#[cfg]`.

See [Standard Library](../std/index.md), [CLI commands](../tooling/commands.md), and [Examples](../examples/overview.md).
