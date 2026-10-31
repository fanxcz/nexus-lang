# Foreign Function Interface (FFI)

The parser supports an external function declaration of the form `extern "C" fn name(parameters) -> return_type`. This is a minimal declaration mechanism, not a complete C FFI: it does not support extern blocks, variadic parameters, function-pointer types, `c"..."` string literals, Rust-style `unsafe` blocks, representation attributes or automatic C string conversion.

## Declaration syntax

```nx
extern "C" fn external_function(value: i64) -> i64

fn call_external(value: i64) -> i64 {
    return external_function(value)
}

fn main() {
    print(call_external(42))
}
```

This example demonstrates the declaration/call shape only. It will link and run only when a definition of `external_function` is available to the selected backend/linker. A declaration by itself does not supply an implementation or discover a library.

## Current type limits

The parser's type system supports fixed-width scalars (`i8`/`i16`/`i32`/`i64`, `u8`/`u16`/`u32`/`u64`, `f32`/`f64`), `bool`, `string`, `void`, pointers written as `*T`, fixed arrays, and user-defined structs/enums. Integer aliases `int`, `isize` and `usize` remain aliases of `i64`, not target-sized C ABI types. Distinct scalar widths do not by themselves make a declaration a complete or automatically verified C binding: struct layout, pointer lifetime and string representation remain the caller's responsibility. The language still does not provide a standard C-string type.

Do not assume the in-memory layout of a NEXUS struct is automatically compatible with an arbitrary C struct. The language currently lacks `#[repr(C)]` and compile-time layout assertions.

## Linking and ABI responsibility

An external declaration does not configure additional library search paths or linker flags by itself. The selected build backend and toolchain must find the symbol; the caller is responsible for matching the real function's calling convention, parameter widths, pointer lifetime and return type.

## Not implemented

The following patterns from older drafts are not accepted by the current parser:

- `extern "C" { ... }` blocks containing many declarations.
- Variadic C functions such as `printf(format, ...)`.
- `*const T` / `*mut T` pointer qualifiers.
- Function pointer / callback parameter types.
- `c"..."`, `.as_c_str()`, `str_from_c_str()`.
- `#[repr(C)]`, other FFI attributes and `unsafe { ... }` semantics.

The FFI surface is experimental. Use it only for signatures the current parser can represent, and verify both linking and runtime behavior on every target you distribute.
