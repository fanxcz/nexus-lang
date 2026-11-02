#!/usr/bin/env python3
"""Measure NEXUS compile and executable times using only the Python stdlib."""

from __future__ import annotations

import argparse
import csv
import json
import os
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nexus", type=Path, default=Path("build/nexus"))
    parser.add_argument("--backends", nargs="+", choices=["native", "llvm", "auto"],
                        default=["native", "llvm"])
    parser.add_argument("--runs", type=int, default=5, help="compile repetitions per backend")
    parser.add_argument("--runtime-runs", type=int, default=5,
                        help="execution repetitions per backend")
    parser.add_argument("--compile-source", type=Path,
                        default=Path("tests/native_programs/arith.nx"))
    parser.add_argument("--runtime-source", type=Path,
                        default=Path("tests/native_programs/loops.nx"))
    parser.add_argument("--json", type=Path, help="write JSON results to this file")
    parser.add_argument("--csv", type=Path, help="write CSV results to this file")
    args = parser.parse_args()
    if args.runs < 1 or args.runtime_runs < 1:
        parser.error("--runs and --runtime-runs must be >= 1")
    return args


def run_checked(command: list[str], *, env: dict[str, str]) -> None:
    result = subprocess.run(command, text=True, capture_output=True, env=env, check=False)
    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )


def measure_compile(nexus: Path, source: Path, backend: str, runs: int,
                    output_dir: Path, env: dict[str, str]) -> list[float]:
    samples: list[float] = []
    suffix = ".exe" if os.name == "nt" else ""
    for index in range(runs):
        executable = output_dir / f"compile-{backend}-{index}{suffix}"
        command = [str(nexus), "build", str(source), "-o", str(executable),
                   "--backend", backend]
        start = time.perf_counter()
        run_checked(command, env=env)
        samples.append(time.perf_counter() - start)
    return samples


def measure_runtime(nexus: Path, source: Path, backend: str, runs: int,
                    output_dir: Path, env: dict[str, str]) -> list[float]:
    suffix = ".exe" if os.name == "nt" else ""
    executable = output_dir / f"runtime-{backend}{suffix}"
    run_checked([str(nexus), "build", str(source), "-o", str(executable),
                 "--backend", backend], env=env)
    samples: list[float] = []
    for _ in range(runs):
        start = time.perf_counter()
        run_checked([str(executable)], env=env)
        samples.append(time.perf_counter() - start)
    return samples


def summarize(samples: list[float]) -> dict[str, Any]:
    return {
        "runs_seconds": [round(value, 6) for value in samples],
        "median_seconds": round(statistics.median(samples), 6),
        "min_seconds": round(min(samples), 6),
        "max_seconds": round(max(samples), 6),
    }


def main() -> int:
    args = parse_args()
    nexus = args.nexus.resolve()
    compile_source = args.compile_source.resolve()
    runtime_source = args.runtime_source.resolve()
    for path in (nexus, compile_source, runtime_source):
        if not path.is_file():
            print(f"error: required file does not exist: {path}", file=sys.stderr)
            return 2

    env = os.environ.copy()
    results: dict[str, Any] = {
        "compiler": str(nexus),
        "compile_source": str(compile_source),
        "runtime_source": str(runtime_source),
        "compile_runs": args.runs,
        "runtime_runs": args.runtime_runs,
        "backends": {},
    }
    try:
        with tempfile.TemporaryDirectory(prefix="nexus-bench-") as temp:
            output_dir = Path(temp)
            for backend in args.backends:
                print(f"Benchmarking backend={backend} ...", flush=True)
                compile_samples = measure_compile(
                    nexus, compile_source, backend, args.runs, output_dir, env
                )
                runtime_samples = measure_runtime(
                    nexus, runtime_source, backend, args.runtime_runs, output_dir, env
                )
                results["backends"][backend] = {
                    "compile": summarize(compile_samples),
                    "runtime": summarize(runtime_samples),
                }
    except (OSError, RuntimeError) as error:
        print(f"benchmark failed: {error}", file=sys.stderr)
        return 1

    print(json.dumps(results, indent=2))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        with args.csv.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle)
            writer.writerow(["backend", "metric", "median_seconds", "min_seconds",
                             "max_seconds", "runs_seconds"])
            for backend, backend_result in results["backends"].items():
                for metric in ("compile", "runtime"):
                    summary = backend_result[metric]
                    writer.writerow([backend, metric, summary["median_seconds"],
                                     summary["min_seconds"], summary["max_seconds"],
                                     ";".join(map(str, summary["runs_seconds"]))])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
