# Good First Issues

These are scoped starter tasks based on existing code and documentation gaps. They are suggestions for issue creation, not claims that GitHub issues already exist. Maintainers should apply `good first issue` or `help wanted` only after confirming scope and acceptance criteria.

| Task | Likely area | Acceptance criteria |
|---|---|---|
| Add a lexer seed for malformed UTF-8 and embedded NUL boundaries | `tests/fuzz/`, lexer tests | Seed is deterministic; lexer reports a diagnostic or returns tokens without crashing |
| Expand native/LLVM parity fixtures for compound arithmetic | `tests/native_programs/`, `tests/differential/` | New program runs on both backends and stdout/exit status match |
| Add a negative test for invalid string escapes | `tests/negative/`, lexer tests | Exact diagnostic and non-zero status are asserted |
| Document `NEXUS_CXX` discovery on Windows PowerShell | `docs/troubleshooting.md` | Commands reproduce compiler discovery and match driver behavior |
| Add a standard-library example for `std.option` | `stdlib/`, `docs/std/option.md`, `examples/` | Runnable example and focused regression test |
| Add benchmark output in CSV format | `benchmarks/run.py` | Stable header, per-backend timings and no new runtime dependency |
| Add a native backend test for a currently supported string edge case | `tests/native_programs/`, `tests/differential/` | Existing native subset remains unchanged; both backends match |
| Improve one CMake diagnostic for a missing optional dependency | `CMakeLists.txt`, `docs/troubleshooting.md` | Configure remains successful when the dependency is intentionally absent |

## Contributor workflow

Read [CONTRIBUTING.md](../CONTRIBUTING.md), follow the [NEXUS style guide](style-guide-nx.md), and run the focused test before opening a PR. Ask usage questions in Discussions; reserve Issues for reproducible bugs and accepted work items.
