#!/usr/bin/env python3
"""Repository-wide formatter and lint gate.

The checkout root carries a ``nexus.toml`` so the compiler treats the whole
repository as one project. This test proves that gate stays usable: every
shipped ``.nx`` source passes ``nexus fmt --check`` and ``nexus lint``, and the
root manifest itself parses.

``selfhost/tests/*.nx`` are program fragments consumed by the self-host suite,
not standalone programs, so they are exempt from the lint pass.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

SKIP_DIRS = {
    ".git", "build", "dist", "site", "vendor", "node_modules", "CMakeFiles",
    "target", ".cache", ".venv",
}
SKIP_LINT = (Path("selfhost") / "tests",)


def sources(root: Path) -> list[Path]:
    found: list[Path] = []
    for path in sorted(root.rglob("*.nx")):
        relative = path.relative_to(root)
        if any(
            part in SKIP_DIRS or part.startswith("build-")
            for part in relative.parts[:-1]
        ):
            continue
        found.append(path)
    return found


def run(nexus: Path, *arguments: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(nexus), *arguments],
        cwd=cwd,
        capture_output=True,
        text=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nexus", type=Path, required=True, help="path to the built compiler")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args()
    root = args.root.resolve()

    manifest = root / "nexus.toml"
    if not manifest.is_file():
        raise SystemExit(f"repository manifest is missing: {manifest}")

    info = run(args.nexus, "info", cwd=root)
    if info.returncode != 0 or "nexus-repository" not in info.stdout:
        raise SystemExit(f"nexus info rejected the repository manifest:\n{info.stdout}{info.stderr}")

    formatted = run(args.nexus, "fmt", "--check", "--all", cwd=root)
    if formatted.returncode != 0:
        raise SystemExit(
            "nexus fmt --check --all failed; run `nexus fmt --all` from the repository "
            f"root:\n{formatted.stdout}{formatted.stderr}"
        )

    all_sources = sources(root)
    linted = 0
    warnings = 0
    for path in all_sources:
        relative = path.relative_to(root)
        if any(relative.parts[: len(prefix.parts)] == prefix.parts for prefix in SKIP_LINT):
            continue
        result = run(args.nexus, "lint", str(path), cwd=root)
        if result.returncode != 0:
            raise SystemExit(f"nexus lint failed for {relative}:\n{result.stdout}{result.stderr}")
        if "NX9001" in result.stdout:
            raise SystemExit(f"{relative}: trailing whitespace is not formatter output")
        warnings += result.stdout.count("NX9002")
        linted += 1

    print(
        f"PASS repository sources: {len(all_sources)} .nx files, {linted} linted, "
        f"{warnings} TODO/FIXME marker(s), formatter clean"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
