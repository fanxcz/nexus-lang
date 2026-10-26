# Application Development with NEXUS

NEXUS is focused first on native desktop applications, command-line utilities, and developer tools. Graphics and games are supported use cases, but you do not need to start with a game project to use the language.

## What is available today

- **Native windowed app template:** creates a small windowed program with an event loop, keyboard/mouse polling, and drawing primitives.
- **CLI template:** a simple starting point for command-line utilities.
- **Application examples:** [NEXUS Paint](https://github.com/fanxcz/nexus-lang/blob/main/examples/nexus_paint.nx), [the native app sample](https://github.com/fanxcz/nexus-lang/blob/main/examples/native_app.nx), and [NEXUS Studio's source](https://github.com/fanxcz/nexus-lang/tree/main/.github/project-tools/nexus-studio).
- **Core libraries:** file operations, filesystem helpers, string operations, networking helpers, JSON access, SQLite, hashing, and memory utilities. Each module has limits documented in the [standard-library reference](std/index.md).
- **Developer workflow:** project templates, checks, builds, run commands, formatting, linting, diagnostics, and editor/LSP support.

These are useful foundations, but they are not yet equivalent to a mature desktop framework such as Qt, .NET desktop UI, or a complete cross-platform widget toolkit.

## Create a native windowed app

    nexus new MyApp --template app
    cd MyApp
    nexus check
    nexus build
    nexus run

The generated template starts from a native window, event loop, keyboard/mouse input, drawing primitives, and clean shutdown. It is suitable for learning the runtime and prototyping a small graphical tool.

For a terminal utility instead:

    nexus new MyTool --template cli
    cd MyTool
    nexus check
    nexus build
    nexus run

## Compile a small command-line program

Create a file named hello.nx:

    fn main() {
        print("Hello from NEXUS!")
    }

Then run it:

    nexus run hello.nx

For programs inside the documented built-in native-backend subset, you can also explicitly choose --backend native. Broader language features may require the LLVM compilation path and its toolchain; see [native backend limitations](tooling/native-backend.md).

## What should come next

The roadmap prioritizes the practical pieces needed for real applications:

1. A consistent UI toolkit with layout, buttons, text inputs, menus, focus handling, and keyboard navigation.
2. OS integration such as file dialogs, clipboard access, configuration paths, process management, and clear error handling.
3. Reproducible packaging of runtime libraries and application resources.
4. Complete sample utilities with tests that build and run from a clean checkout.

These are roadmap goals, not claims that those features are already implemented. Follow [the NEXUS roadmap](development/roadmap.md) for implementation status and platform limits.
