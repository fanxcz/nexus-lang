# Examples

The repository's `examples/` directory contains standalone `.nx` programs and assets. These are runnable source examples, not examples fetched from a package registry.

## In This Section

| Topic | Description |
|---|---|
| [Overview](overview.md) | Index of examples in the source tree |
| [Native app sample](../../examples/native_app.nx) | Minimal native window and interactive drawing sample |
| [Project templates](../templates/index.md) | Starter projects for applications, CLI tools, games, and libraries |
| [Games](games.md) | Terminal and graphical game prototypes |
| [Graphics](graphics.md) | Valid 2D, 3D and drawing examples |
| [Audio](audio.md) | The current sound-runtime demo |
| [Application UI Playground](../../examples/ui_playground.nx) | Immediate-mode UI widgets and layout |
| [2D Starter](../../examples/game2d_starter.nx) | Asset-free tilemap/physics/camera/animation demonstration |

## Run an example

From the repository root (or any environment where the compiler can find the source tree and runtime), pass the source path directly:

```bash
nexus check examples/hello.nx
nexus run examples/hello.nx
```

There is no `nexus run --example NAME` option in the current CLI. For a project scaffold, use `nexus new MyProject --template app` and then run the project commands from its directory.

Graphical examples need a graphics-capable session and the required runtime libraries. Audio and resource demos may require optional host dependencies and the working directory to remain the repository root so relative asset paths resolve.
