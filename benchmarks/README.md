# Compiler and Runtime Benchmarks

The runner reports (1) end-to-end compile time for a representative arithmetic source file and (2) execution time for a deterministic loop-heavy executable. It uses Python's standard library and the compiler's existing native/LLVM backends; no benchmark framework is required.

## Run locally

Build the compiler first, then run from the repository root:

```sh
python3 benchmarks/run.py --nexus ./build/nexus --backends native llvm --runs 5 --runtime-runs 5
```

On Windows use `--nexus .\\build\\nexus.exe`. The LLVM backend requires a compatible C++ compiler; set `NEXUS_CXX` when it is not discoverable as `clang++`.

Optional flags:

- `--backends`: one or more of `native`, `llvm`, `auto`.
- `--runs`: compile repetitions (default 5).
- `--runtime-runs`: executable repetitions (default 5).
- `--json`: output file for machine-readable results.
- `--csv`: output file for a flat table.
- `--compile-source` / `--runtime-source`: override default fixtures.

The default inputs are established deterministic programs in `tests/native_programs/arith.nx` and `tests/native_programs/loops.nx`. This avoids introducing a second benchmark language dialect or duplicating test fixtures.

## Interpretation

The compile measurement includes CLI startup, front-end work, code generation and the backend compiler/linker. Runtime measurements include process startup and program execution. Compare results only on the same machine, OS, compiler/toolchain, build type and environment. Report medians and retain raw runs for serious performance claims. These are regression indicators, not a statistically controlled microbenchmark suite.

## CI

The **Benchmarks** workflow is manual (`workflow_dispatch`) and uploads JSON/CSV results as an artifact. It is intentionally separate from required pull-request checks so noisy performance measurements do not make correctness CI flaky.
