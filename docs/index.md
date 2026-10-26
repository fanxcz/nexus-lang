---
hide:
  - toc
---

<div class="nexus-hero" markdown>

# NEXUS — native programming, clear and practical

A compiled, statically checked language for command-line tools, native applications and 2D games. Start with a tiny program, use the standard library, and build native applications when you're ready.

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

    Explore I/O, files, strings, JSON, networking, math and modular 2D-game helpers.

    [Library modules](std/index.md)

-   :material-gamepad-variant-outline: **Build 2D games**

    See the runtime graphics API, tilemap storage, collision helpers, sprites, frame animation, camera and audio wrappers.

    [2D game libraries](runtime/game2d.md)

-   :material-download-box-outline: **Downloads**

    Windows setup, portable compiler bundle, NEXUS Paint installer, VS Code extension and checksum files.

    [Official downloads](downloads.md)

-   :material-bug-outline: **Compiler and tooling**

    Check diagnostics, backend choice, formatter, linter, LSP, project templates and self-tests.

    [Tooling reference](tooling/index.md)

</div>

## What is implemented today

- **Compiler:** C++20 front end, semantic checks, LLVM/Clang compilation path, and a built-in x86-64 backend for a documented subset.
- **Runtime:** console/file helpers, UTF-8 strings, SDL-backed 2D drawing, optional 3D and audio, and small game-oriented helpers.
- **Developer tools:** project templates, formatter/linter, diagnostics, language server, REPL, test suite and portable Windows packaging.
- **Gaming libraries:** separate modules for math, 2D window/input/drawing, tilemaps, collisions, sprites, animation, camera and audio.

NEXUS is actively developed. The built-in native backend does not support every language feature, and the game helpers are not a complete rigid-body engine. The [language overview](language/overview.md) and [platform support matrix](development/platform-support.md) describe current limits.

## Documentation and community

[Manual](manual.md) · [Examples](examples/overview.md) · [Roadmap](development/roadmap.md) · [GitHub Issues](https://github.com/fanxcz/nexus-lang/issues)
