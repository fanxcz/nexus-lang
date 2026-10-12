# Cross-compilation

NEXUS supports multiple targets through its compiler interface, but the built-in machine-code emitter and the LLVM compilation path have different capabilities. A target name being recognized does not mean every backend can emit it.

## Built-in native backend

The direct native emitter currently produces x86-64 Windows PE executables and x86-64 Linux ELF executables for a documented subset of the language. It does not currently emit macOS Mach-O, ARM64, Android, or WebAssembly binaries.

Examples:

```sh
nexus build app.nx -o app --target x86_64-linux --backend native
nexus build app.nx -o app.exe --target x86_64-windows --backend native
```

These commands work only when the source fits the built-in backend's supported subset. Unsupported features should produce a diagnostic rather than a usable binary.

## LLVM backend

Programs outside the built-in subset, and targets handled by LLVM, require a compatible C++ compiler/toolchain. The Windows portable package bundles a Zig-based C++ compiler wrapper for that workflow. Other platforms may need to install and configure Clang/LLVM or set `NEXUS_CXX`.

Use `--backend llvm` to request the LLVM path explicitly:

```sh
nexus build app.nx -o app --backend llvm
```

Successful cross-compilation depends on the target being supported by the current NEXUS compiler path and by the installed or bundled toolchain, including its target libraries and linker.

## Inspect capabilities first

```sh
nexus targets
nexus explain-backend app.nx
nexus doctor
```

Read the [built-in native backend reference](../tooling/native-backend.md) for the exact subset, or see [targets](targets.md) for how to interpret the target listing.
