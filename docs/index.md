---
hide:
  - toc
---

<div class="nexus-hero" markdown>

# NEXUS — native programming, clear and practical

A compiled, statically checked language for native applications, developer tools, and game prototypes. The compiler now includes fixed-width numeric types and checked slices; SDL-based 2D helpers and an experimental compatibility-OpenGL 3D path provide a starting point for graphics projects.

[Start learning](getting-started/index.md){ .md-button .md-button--primary }
[Download NEXUS](downloads.md){ .md-button }
[View source on GitHub](https://github.com/fanxcz/nexus-lang){ .md-button }
[Download language manual PDF](https://fanxcz.github.io/nexus-lang/pdf/NEXUS-Documentation.pdf){ .md-button }

</div>

<div class="nexus-kicker">Start here</div>

## Build your first program

Create `hello.nx`:

```nx
fn main() {
    let name = "NEXUS"
    print("Hello from " + name + "!")
}
```

Run it from your terminal:

```powershell
nexus check hello.nx
nexus run hello.nx
```

The compiler checks source code before building. Run `nexus doctor` if the compiler, toolchain or runtime is not available.

## Explore NEXUS

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **Get started**

    Install the compiler, create a project, and learn the basic workflow.

    [Quick Start](getting-started/quickstart.md)

-   :material-language-c: **Language reference**

    Learn variables, types, functions, arrays, structs, enums, loops, imports and the current feature boundaries.

    [Syntax and examples](language/syntax.md)

-   :material-package-variant-closed: **Standard library**

    Explore files, strings, JSON, networking, math, collections, UI layout and runtime helpers.

    [Library modules](std/index.md)

-   :material-desktop-classic: **Build native apps**

    Create a windowed app or CLI utility, use immediate-mode widgets, and explore the current runtime and development workflow.

    [Application development](applications.md)

-   :material-gamepad-variant-outline: **Game development**

    Explore tilemaps, collision helpers, sprites, animation, cameras, and the experimental 3D API.

    [Game development status](runtime/voxel-roadmap.md)

-   :material-download-box-outline: **Downloads**

    Windows setup, portable compiler bundle, NEXUS Paint installer, VS Code extension and checksum files.

    [Official downloads](downloads.md)

-   :material-bug-outline: **Compiler and tooling**

    Check diagnostics, backend choice, formatter, linter, LSP, project templates and self-tests.

    [Tooling reference](tooling/index.md)

</div>

## Featured projects

<div class="grid cards" markdown>

-   **Vector Lab**

    A small playable arena that demonstrates vector math, obstacle collision, delta-time movement and animation curves.

    [Open the source](https://github.com/fanxcz/nexus-lang/blob/main/examples/vector_lab.nx)

-   **NEXUS Paint**

    A native raster-painting example built with the graphics and UI runtime.

    [Open the source](https://github.com/fanxcz/nexus-lang/blob/main/examples/nexus_paint.nx)

</div>

## A practical development workflow

Start with a small program, check it, then choose the compiler backend explicitly
when you need to control which toolchain builds the executable.

<div class="grid cards" markdown>

-   **Command-line utility**

    Keep the first project simple and portable.

    `nexus new MyTool --template cli`

-   **Native windowed app**

    Try the current widget and layout layer.

    `nexus new MyApp --template app`

-   **Interactive UI sample**

    Explore panels, checkboxes, buttons, sliders and responsive layout helpers.

    [Open UI Playground source](https://github.com/fanxcz/nexus-lang/blob/main/examples/ui_playground.nx)

</div>

## What is implemented today

- **Compiler:** C++20 front end, fixed-width integer/floating-point types, checked read-only and mutable slices, LLVM/Clang compilation path, and a built-in x86-64 backend for a documented subset.
- **Runtime:** console/file helpers, UTF-8 strings, SDL-backed 2D drawing, experimental compatibility-OpenGL 3D, optional audio, and immediate-mode UI widgets.
- **Developer tools:** project templates, formatter/linter, diagnostics, language server, REPL, test suite and portable Windows packaging.
- **Application and graphics libraries:** typed math helpers, UI layout/controls, file/network/database utilities, and modular tilemap/sprite/camera/audio helpers.

For voxel-game development, the compiler now has useful language foundations, but NEXUS does not yet ship a production voxel engine. Chunk meshing/batching, GPU buffer management, modern shader support, job scheduling, procedural noise, and compressed chunk persistence remain roadmap work. See the [3D and voxel development status](runtime/voxel-roadmap.md).

NEXUS is actively developed. The built-in native backend supports a documented subset of the language; broader programs may need LLVM/Clang. The UI module is immediate-mode rather than a full widget framework, and the 2D physics helpers are not a rigid-body engine. The [language overview](language/overview.md) and [platform support matrix](development/platform-support.md) describe current limits.

## Documentation and community

[Manual](manual.md) · [Examples](examples/overview.md) · [Roadmap](development/roadmap.md) · [GitHub Issues](https://github.com/fanxcz/nexus-lang/issues)
