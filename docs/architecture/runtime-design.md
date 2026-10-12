# Runtime Design

The NEXUS runtime is a native C/C++ layer linked with generated program code. Its public surface is exposed to NEXUS programs through runtime built-ins.

## Subsystems

- **Platform and windowing:** window creation, events, input, and timing.
- **Rendering:** 2D rendering through SDL2 and optional OpenGL-backed 3D features.
- **Audio:** sound and music helpers.
- **Gameplay:** entities and components, scenes, particles, and physics helpers.
- **Utilities:** file access, environment information, lightweight JSON helpers, hashing, and SQLite integration where supported.

## Initialization and lifecycle

Applications should initialize only the subsystems they use, run rendering and input on the main thread, and release resources before exit. The runtime is not currently documented as thread-safe.

## Platform dependencies

Core language compilation uses LLVM/Clang. Graphics features require the matching platform libraries; a successful compiler build does not by itself guarantee that every optional runtime feature is installed.

See the [runtime API overview](../runtime/overview.md) for module documentation and examples.
