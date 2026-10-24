# Benchmarking

`nexus bench` measures the elapsed time of repeatedly starting a compiled NEXUS program. It is a command-line smoke/performance measurement, not a framework for benchmarking individual functions.

## Usage

```sh
# Measure the project's configured entry program (requires nexus.toml)
nexus bench

# Measure a source file
nexus bench src/main.nx

# Execute the built program ten times
nexus bench src/main.nx --runs 10
```

The default is five runs. NEXUS compiles the source once to a temporary executable, executes it repeatedly, and prints the average elapsed time in microseconds. If a run returns a nonzero exit code, the benchmark fails.

## Interpreting results

The measurement includes process startup and all work performed by the program. Very short programs can be dominated by operating-system scheduling and process-launch overhead. Compare results on the same machine, use enough runs, and avoid treating a single result as a reliable performance claim.

## Limitations

The current command does not support named benchmark functions, a warmup count, JSON/CSV output, baselines, statistical confidence intervals, throughput metrics, or an embedded benchmark API. The `--bench`, `--warmup`, `--format`, and `--baseline` flags in older documentation are not implemented.

See [Tooling Commands](commands.md) for the exact command syntax and the [roadmap](../development/roadmap.md) for future benchmarking support.
