---
hide:
  - toc
---

<div class="nexus-hero" markdown>

<div class="nexus-hero-copy" markdown>

<div class="nexus-kicker">NEXUS 2.2.0 · C++20 toolchain</div>

# Native software, from language to runtime.

NEXUS is a statically checked programming language for native applications, command-line tools and graphics experiments. Explore a practical compiler, a built-in x86-64 backend for a defined subset, an LLVM path for broader programs, and a growing set of developer tools.

[Explore the language](getting-started/index.md){ .md-button .md-button--primary }
[Download NEXUS](downloads.md){ .md-button }
[Read the documentation](manual.md){ .md-button }
[View on GitHub](https://github.com/fanxcz/nexus-lang){ .md-button }

<div class="nexus-hero-meta">
  <span>Static checking</span>
  <span>LLVM + native x86-64</span>
  <span>Windows · Linux · macOS CI</span>
</div>

</div>

<div class="nexus-hero-code" markdown>

<div class="nexus-code-heading"><span>hello.nx</span><span>FIRST PROGRAM</span></div>

```nx
fn greet(name: string) -> string {
    return "Hello, " + name + "!"
}

fn main() {
    let message = greet("NEXUS")
    print(message)
}
```

<div class="nexus-code-caption">Write a small program. Check it. Build and run with the NEXUS CLI.</div>

</div>

</div>

<div class="nexus-kicker">The toolchain</div>

## One language, connected tools

NEXUS brings the language, compiler, runtime and editor integrations into one repository. Each component has a defined boundary, and current backend and platform limits are documented rather than hidden.

<div class="grid cards" markdown>

-   :material-code-braces: **Language and compiler**

    Position-aware lexer, recursive-descent parser, shared AST, semantic checks and two compilation paths.

    [Compiler pipeline](architecture/compiler-pipeline.md)

-   :material-lightning-bolt-outline: **Native execution**

    A built-in x86-64 backend for its documented subset, plus an LLVM/Clang path for broader language coverage.

    [Backend selection](tooling/backend-explainer.md)

-   :material-cube-outline: **Runtime and library**

    Strings, memory, file and data helpers, SDL-based graphics and optional native integrations.

    [Runtime overview](runtime/index.md)

-   :material-lan: **Developer tooling**

    CLI, REPL, formatter, linter, diagnostics, language server and VS Code language support.

    [Tooling guide](tooling/index.md)

</div>

## Get productive

<div class="nexus-quickstart" markdown>

### Build your first program

Create `hello.nx`:

```nx
fn main() {
    let name = "NEXUS"
    print("Hello from " + name + "!")
}
```

Run it from a terminal:

```sh
nexus check hello.nx
nexus run hello.nx
```

The compiler checks the source before building. Run `nexus doctor` when the compiler, toolchain or runtime is not available.

[Follow the quick start](getting-started/quickstart.md){ .md-button .md-button--primary }

</div>

## Choose your workflow

<div class="grid cards" markdown>

-   :material-console: **Command-line projects**

    Create a small CLI utility, use compiler diagnostics and build from a terminal.

    `nexus new MyTool --template cli`

    [CLI guide](tooling/commands.md)

-   :material-monitor: **Native applications**

    Start from the current application template and use the available runtime UI and graphics APIs.

    `nexus new MyApp --template app`

    [Application development](applications.md)

-   :material-code-editor: **VS Code**

    Open `.nx` files with syntax highlighting, snippets and the NEXUS language server client. Install the compiler separately.

    [Extension guide](tooling/vscode-extension.md)

-   :material-application-brackets-outline: **NEXUS Studio**

    Use the source-buildable Electron editor, project explorer, terminal and compiler Build/Run integration.

    [Studio source and build instructions](https://github.com/fanxcz/nexus-lang/tree/main/.github/project-tools/nexus-studio)

</div>

## Examples in the repository

<div class="grid cards" markdown>

-   **Vector Lab**

    A small playable arena demonstrating vector math, obstacle collision, delta-time movement and animation curves.

    [Explore the source](https://github.com/fanxcz/nexus-lang/blob/main/examples/vector_lab.nx)

-   **NEXUS Paint**

    A raster-painting example with brush, eraser, line and rectangle tools, fill, undo/redo and BMP file operations.

    [Explore the source](https://github.com/fanxcz/nexus-lang/blob/main/examples/nexus_paint.nx)

-   **UI Playground**

    A runnable sample for the current immediate-mode controls and layout helpers.

    [Explore the source](https://github.com/fanxcz/nexus-lang/blob/main/examples/ui_playground.nx)

</div>

## What is implemented today

| Area | Current capability | Boundary |
|---|---|---|
| Compiler | C++20 frontend, fixed-width numeric types, checked slices | Native backend supports a documented subset; broader programs may require LLVM/Clang |
| Runtime | Console and file helpers, UTF-8 strings, memory and data APIs | Graphics/audio depend on optional native libraries |
| Graphics | SDL-backed 2D helpers and compatibility-OpenGL 3D path | 3D path is experimental; no production voxel engine |
| Language services | Diagnostics, completion, hover, navigation, rename, formatting and quick fixes | LSP workspace/type information has documented lexical limitations |
| Editors | VS Code extension and source-buildable NEXUS Studio | Studio does not yet embed the LSP; compiler is installed separately |
| Distribution | Windows setup and portable packages, Paint installer and VSIX | Platform availability is artifact-specific; a CI host build is not a published installer |

NEXUS does not yet ship a production voxel engine. Chunk meshing/batching, GPU buffer management, modern shader support, job scheduling, procedural noise and compressed chunk persistence remain roadmap work. The UI layer is immediate-mode rather than a full widget framework, and the 2D physics helpers are not a rigid-body engine.

## Explore the platform

<div class="grid cards" markdown>

-   **Download Center**

    Verified compiler and application artifacts, setup guidance and SHA-256 files.

    [Choose a download](downloads.md)

-   **Language reference**

    Learn syntax, types, functions, modules and error handling.

    [Read the language guide](language/index.md)

-   **Architecture**

    Understand how the frontend, semantic analysis, backends, runtime and tools fit together.

    [Explore the ecosystem architecture](architecture/ecosystem.md)

-   **Development**

    Build from source, run tests, inspect platform support and contribute.

    [Developer guide](development/index.md)

</div>

NEXUS is actively developed. Consult the [platform support matrix](development/platform-support.md), [language reference](language/overview.md) and [roadmap](development/roadmap.md) before relying on a feature for production.
