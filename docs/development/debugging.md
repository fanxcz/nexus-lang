# Debugging

This page documents workflows supported by the current NEXUS compiler. NEXUS does not yet provide a source-level debugger protocol, source maps, or the language/runtime features shown in some generic debugging examples (such as string interpolation, `eprintln`, assertion macros, or `file!`/`line!` macros).

## Diagnose a source file

Run the compiler checks before starting an application:

```sh
nexus check src/main.nx
nexus explain-backend src/main.nx
nexus doctor
```

Use `nexus explain-backend` to understand whether a program fits the built-in machine-code subset or needs the LLVM path. `nexus doctor` checks the available compiler/toolchain configuration.

## Inspect generated code

The LLVM path can emit LLVM IR for inspection:

```sh
nexus build src/main.nx -o build/main --emit-ir
```

The native backend emits PE/ELF machine code directly and does not produce LLVM IR.

## Print-based debugging

Use ordinary `print` calls and string concatenation:

```nx
fn calculate(value: i64) -> i64 {
    print("calculate input = " + value)
    let result = value * 2
    print("calculate result = " + result)
    return result
}

fn main() {
    let answer = calculate(21)
    print("answer = " + answer)
}
```

NEXUS automatically converts `i64`, `f64`, and `bool` values to strings when concatenated with a string. There is no general-purpose format-string interpolation.

## Native debugger

You can use GDB or LLDB to inspect the generated executable at the machine-code level:

```sh
gdb ./build/main
```

or:

```sh
lldb ./build/main
```

Debugger support is currently limited to what the generated executable and toolchain provide. Source-level breakpoints in `.nx` files, a NEXUS debug adapter, variable mapping back to source names, and integrated IDE debugging are not implemented.

## Sanitizers

The repository's `sanitizers.yml` workflow builds and tests the C++ compiler/frontend/runtime with AddressSanitizer and UndefinedBehaviorSanitizer. To reproduce that workflow on Linux with Clang installed:

```sh
cmake -S . -B build-sanitize -G Ninja \
  -DNEXUS_BUILD_TESTS=ON \
  -DNEXUS_USE_CLANG=OFF \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-sanitize --parallel 2
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-sanitize --output-on-failure
```

These sanitizer flags instrument the C++ tools being built. They do not automatically instrument a separately generated NEXUS executable.

## Run the test suite

```sh
ctest --test-dir build --output-on-failure
```

For changes to the runtime JSON parser, run its focused test:

```sh
ctest --test-dir build -R nexus_runtime_json_test --output-on-failure
```
