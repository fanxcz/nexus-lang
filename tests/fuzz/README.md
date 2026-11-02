# Frontend libFuzzer Targets

This directory contains independent libFuzzer entry points for the lexer and parser. The lexer target exercises tokenization; the parser target tokenizes arbitrary bytes and parses the resulting token stream. Malformed input is expected to produce an ordinary diagnostic. A crash, sanitizer finding or timeout is a bug.

## Requirements and build

Use a Clang toolchain with compiler-rt/libFuzzer support. GCC and MSVC are not supported by these targets. Fuzz targets are opt-in so normal builds do not change:

```sh
cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DNEXUS_BUILD_TESTS=ON \
  -DNEXUS_BUILD_LIBFUZZER=ON
cmake --build build-fuzz --target nexus_fuzz_lexer nexus_fuzz_parser
ctest --test-dir build-fuzz --output-on-failure
```

## Run longer campaigns

```sh
./build-fuzz/nexus_fuzz_lexer -max_len=4096 -max_total_time=60 tests/fuzz/corpus/lexer
./build-fuzz/nexus_fuzz_parser -max_len=4096 -max_total_time=60 tests/fuzz/corpus/parser
```

Use separate writable corpus/output directories for longer runs so checked-in seed files remain unchanged. Preserve every crashing input as a regression seed after minimizing and reviewing it.

## Scope and limits

The harness targets lexer and parser, not semantic analysis, code generation or runtime. The existing `nexus_frontend_fuzz_smoke` CTest is deterministic and remains enabled in normal test builds. Fuzzing is not a proof of safety; run longer campaigns under sanitizers and report reproducible security-impacting failures privately.
