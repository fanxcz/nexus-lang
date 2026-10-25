# Quick Start

This guide uses the command-line workflow and runtime API included with NEXUS 2.0.0. Start with a small console program, then try a native graphical app.

## 1. Check the compiler

Open a terminal and run:

```bash
nexus --version
nexus doctor
```

On Windows, use PowerShell or Windows Terminal. If `nexus` is not found, follow the [installation guide](installation.md).

### Interactive calculations

Run `nexus` with no arguments, or use `nexus repl`, to evaluate expressions in the NEXUS language:

```text
nexus> 2 + 2
4
nexus> let mut answer = 21
nexus> answer * 2
42
nexus> answer = answer + 1
nexus> answer
22
```

Use `:help` for commands, `:vars` to list saved bindings, `:clear` to reset the session and `:quit` to exit. This is a NEXUS evaluator, not a Python interpreter.

## 2. Create and run a command-line project

```bash
nexus new HelloNexus --template cli
cd HelloNexus
nexus check
nexus build
nexus run
```

NEXUS creates a project directory with a manifest and source file. Run commands from that project directory so the compiler can find its settings.

A small NEXUS program looks like this:

```nx
fn main() {
    print("Hello from NEXUS!")
}
```

## 3. Create a native graphical app

NEXUS includes an app template using the runtime's graphics API. Create it in a separate directory:

```bash
nexus new MyApp --template app
cd MyApp
nexus check
nexus build
nexus run
```

The template is a complete working example. Its core loop uses graphics functions present in the repository:

```nx
fn main() {
    let ok = gfx_init(1000, 640, "NEXUS App")
    if !ok {
        print("Graphics backend unavailable: " + gfx_error())
        return
    }

    while !gfx_should_close() {
        gfx_poll()
        if gfx_key_down("ESC") {
            break
        }

        gfx_begin()
        gfx_clear(0.035, 0.045, 0.08, 1.0)
        gfx_rect(40.0, 40.0, 920.0, 110.0, 0.07, 0.12, 0.22, 1.0)
        gfx_text(70.0, 70.0, 3.0, "NEXUS APP", 0.55, 0.9, 1.0, 1.0)
        gfx_text(70.0, 120.0, 1.5, "ESC closes the window", 0.8, 0.84, 0.95, 1.0)
        gfx_end()
    }

    gfx_shutdown()
}
```

This example is based on `examples/templates/app.nx`. Graphics require the runtime dependencies supplied by the Windows portable package or installed for your platform/build.

## 4. Explore other templates

| Template | Use it for |
|---|---|
| `cli` | Command-line utilities |
| `app` | Windowed native applications |
| `game` | 2D game starting point |
| `3d` | 3D graphics starting point |
| `paint` | Raster-canvas drawing application |

| `server` | Server experiments |
| `library` | Reusable source package |

The experimental editor demo is not a template accepted by `nexus new`. See [Editor source](../templates/editor.md) for the tested way to try it. For example, `nexus new MyPaint --template paint` creates a Paint application scaffold. See [Templates](../templates/index.md) and [Examples](../examples/overview.md) for details.

## 5. Useful commands

| Command | Description |
|---|---|
| `nexus check` | Check the current project or a source file |
| `nexus build` | Build the current project |
| `nexus build --release` | Build an optimized project |
| `nexus run` | Build and run the current project |
| `nexus doctor` | Diagnose the compiler/runtime setup |
| `nexus self-test` | Compile and run the compiler's small self-test |
| `nexus explain-backend src/main.nx` | Explain which backend will be used |

Use `nexus --help` to list the supported CLI commands. Command-specific `--help` flags are not currently implemented. The built-in native backend supports a defined x86-64 Linux/Windows subset; other programs may require the LLVM/Clang path. See [Backend selection](../tooling/backend-explainer.md).

## Next steps

- Read the [Language Reference](../language/overview.md).
- Explore the [Standard Library](../std/overview.md).
- Learn the [Runtime API](../runtime/overview.md).
- Browse [Examples](../examples/overview.md).
- Read [Building from Source](../development/building.md).
