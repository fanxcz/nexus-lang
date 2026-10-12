# CLI Template

Command-line application template.

## Create

```bash
nexus new MyCLI --template cli
cd MyCLI
nexus build
nexus run -- --help
```

## Structure

```
MyCLI/
├── nexus.toml
├── src/
│   ├── main.nx       # Entry + argument parsing
│   ├── commands/
│   │   ├── mod.nx
│   │   ├── hello.nx
│   │   └── version.nx
│   └── utils.nx
├── tests/
└── examples/
```

## Features

- Argument parsing (flags, options, positional)
- Subcommands (`mycli command --option`)
- Help generation (`--help`)
- Version flag (`--version`)
- Colored output
- Config file support

## Example Usage

```bash
$ mycli hello --name Alice
Hello, Alice!

$ mycli hello
Hello, World!

$ mycli version
mycli 1.0.0

$ mycli --help
Usage: mycli [OPTIONS] <COMMAND>

Commands:
  hello     Say hello
  version   Show version
  config    Manage config

Options:
  -h, --help     Show help
  -v, --version  Show version
```

## Argument Parsing

```nx
// src/main.nx
import std.os
import std.io

fn main() {
    let args = os.args()
    
    if args.len < 2 {
        print_help()
        return
    }
    
    match args[1] {
        "hello" => cmd_hello(&args[2..]),
        "version" => println("{}", env!("CARGO_PKG_VERSION")),
        "config" => cmd_config(&args[2..]),
        _ => {
            eprintln("Unknown command: {}", args[1])
            print_help()
        }
    }
}

fn cmd_hello(args: []str) {
    let name = if args.len > 0 { args[0] } else { "World" }
    println("Hello, {}!", name)
}
```

## Adding Subcommands

```nx
// src/commands/new_command.nx
module commands::new_command {
    export fn run(args: []str) {
        // Implementation
    }
    
    export const HELP = "Description of command"
}
```

Register in `src/commands/mod.nx`:
```nx
module commands {
    export module hello
    export module version
    export module new_command
}
```