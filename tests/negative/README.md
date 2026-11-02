# Negative Tests

`run.cmake` drives the real `nexus check` command against malformed source, integer/array-length overflow, an import cycle and invalid file inputs. Each case must fail with compiler diagnostic exit status `2` and a stable diagnostic substring; a crash or accepted invalid input fails the test.

Run after a normal build:

```sh
ctest --test-dir build -R nexus_negative_tests --output-on-failure
```

When a diagnostic intentionally changes, update the expected text alongside the compiler change and changelog entry.
