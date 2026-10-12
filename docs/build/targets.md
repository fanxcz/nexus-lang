# Targets and backend support

Use the compiler itself as the source of truth for the target names available in the current build:

```sh
nexus targets
```

The command lists target names recognized by NEXUS. Backend support is a separate question: the built-in emitter currently covers only x86-64 Windows and Linux executable formats, and only for the language subset described in the [native backend guide](../tooling/native-backend.md). The LLVM backend depends on the configured Clang/C++ toolchain and its support for the selected target.

| Target family | Built-in native emitter | Notes |
|---|---|---|
| x86-64 Windows | Supported subset | Emits PE executable images; source must fit the native backend subset |
| x86-64 Linux | Supported subset | Emits ELF executable images; source must fit the native backend subset |
| macOS / ARM64 / Android / WebAssembly | Not emitted by built-in backend | Requires a compatible LLVM path where supported; availability is not guaranteed by the target name alone |

## Select a target and backend

```sh
nexus build app.nx -o app --target x86_64-linux --backend native
nexus explain-backend app.nx --target x86_64-linux
```

Use the target spelling accepted by `nexus targets` for your build. If a backend cannot emit the selected target or handle a source construct, NEXUS should report the limitation instead of silently implying that a binary was generated successfully.
