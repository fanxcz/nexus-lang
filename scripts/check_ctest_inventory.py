#!/usr/bin/env python3
"""Fail CI when required quality-gate tests are no longer registered with CTest."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

REQUIRED_TESTS = {
    "nexus_negative_tests",
    "nexus_differential_smoke",
    "nexus_fuzz_lexer_smoke",
    "nexus_fuzz_parser_smoke",
    "nexus_frontend_fuzz_smoke",
}


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} <build-directory>", file=sys.stderr)
        return 2

    result = subprocess.run(
        ["ctest", "--test-dir", sys.argv[1], "-N"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        return result.returncode

    registered = set(re.findall(r"^\s*Test\s+#\d+:\s+(\S+)\s*$", result.stdout, re.MULTILINE))
    missing = sorted(REQUIRED_TESTS - registered)
    if missing:
        print("Required CTest registrations are missing:", file=sys.stderr)
        for name in missing:
            print(f"  - {name}", file=sys.stderr)
        return 1

    print(f"CTest inventory OK: {len(registered)} tests registered.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
