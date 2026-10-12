# CLI Template

The `cli` template is a tiny interactive command-line starter. The checked-in `examples/templates/cli.nx` asks for the user's name and prints a greeting. It does not provide a command parser, subcommands, automatic help text or configuration-file handling.

## Create and run

```bash
nexus new MyCli --template cli
cd MyCli
nexus check
nexus build
nexus run
```

## Included program

```nx
fn main() {
    print("NEXUS application")
    let name = input("Your name: ")
    print("Hello, " + name)
}
```

## Add arguments

The runtime currently exposes `os.args_count()` and `os.args_get(index)` built-ins, wrapped by `std.os` as `args_count()` and `args_get(index)`. For prompted input, `std.io` contains `read_line(prompt)`, `read_i64(prompt)` and `read_f64(prompt)`. See [std.os](../std/os.md) and [std.io](../std/io.md).

The generated project is a starter source file. Add command routing, validation, tests and error handling explicitly as the application grows.
