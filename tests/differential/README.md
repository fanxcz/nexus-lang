# Native vs LLVM Differential Tests

The differential runner compiles the same deterministic `.nx` fixtures with `--backend native` and `--backend llvm`, runs both outputs, then compares exit status and normalized stdout. Existing fixtures under `tests/native_programs/` remain the single source of truth.

The runner requires an x86-64 Windows or Linux host with the builtin emitter and a compatible `clang++` driver for the LLVM path. CTest registers it only under the same platform/toolchain conditions as the existing `nexus_backend_parity` test.

```sh
cmake -S . -B build -G Ninja -DNEXUS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -R 'nexus_(backend_parity|differential_smoke)' --output-on-failure
```

Use deterministic fixtures only: no stdin, network, current time, graphics/audio devices or host-specific resource paths. When a feature intentionally differs by backend support, test it in a backend-specific suite rather than asserting parity.
