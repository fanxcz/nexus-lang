# Backend selection and parity

NEXUS has two compilation paths. The built-in backend emits x86-64 Windows PE or Linux ELF machine code directly for a defined subset. The LLVM backend supports the broader language and target set, but needs a compatible C++ compiler such as <code>clang++</code> (or a configured <code>NEXUS_CXX</code> / bundled compiler).

## Explain a build before running it

<pre><code>nexus explain-backend src/main.nx
nexus explain-backend src/main.nx --target x86_64-linux
nexus doctor</code></pre>

The report shows the resolved target, the backend selected by <code>auto</code>, and the reason for using either the built-in backend or LLVM fallback. The command performs normal parsing and semantic checks; it does not build or run the program.

## What is tested

On x86-64 Linux and Windows, CI compares stdout and exit status from representative programs compiled with <code>--backend native</code> and <code>--backend llvm</code>. The current parity set covers arithmetic, function calls, loops, a wide stack frame, type printing, and strings. macOS CI validates the project but does not run the built-in machine-code parity suite because this backend has no macOS emitter yet.

## Current limitations

The built-in backend is intentionally a subset, not a replacement for LLVM yet. Floating-point values, arrays, structs, enums, pattern matching, pointers, and runtime graphics/audio/network APIs are not currently supported by the built-in emitter. A native rejection should report the unsupported construct rather than emit a broken executable.

If the report selects LLVM unexpectedly, first inspect the reason, then use <code>nexus doctor</code> to check compiler discovery.
