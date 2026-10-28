#!/usr/bin/env python3
"""Download the official NEXUS Windows installer and prove it is usable.

CI consumes the installer published on GitHub Pages. Pages answers a request
for a file that has not been deployed yet with an HTTP 200 HTML error page, so
a naive download stores a web page as ``NEXUS-Setup.exe`` and only fails much
later with a confusing setup error. This helper retries while a deployment
propagates, then rejects any payload that is not a Windows PE executable and
verifies it against the published SHA-256 checksum.

Exit codes:
    0  installer downloaded and verified
    2  the downloaded payload is not a Windows PE executable
    3  the published checksum is malformed or does not match the payload
    4  the download failed on every attempt
"""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from validate_downloads import pe_problem  # noqa: E402

DEFAULT_BASE_URL = "https://fanxcz.github.io/nexus-lang/assets/downloads"
SHA256_LINE = re.compile(r"([0-9a-fA-F]{64})(?:\s+(.+))?")


def download(url: str, destination: Path, timeout: float) -> None:
    with urllib.request.urlopen(url, timeout=timeout) as response:
        destination.write_bytes(response.read())


def verify_checksum(installer: Path, checksum: Path, name: str) -> str | None:
    """Return a human-readable reason the checksum does not cover the payload."""
    recorded = checksum.read_text(encoding="ascii", errors="replace").strip()
    match = SHA256_LINE.fullmatch(recorded)
    if not match:
        return f"{name}.sha256.txt does not contain a SHA-256 digest"
    actual = hashlib.sha256(installer.read_bytes()).hexdigest()
    if actual != match.group(1).lower():
        return f"{name} does not match {name}.sha256.txt"
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True, help="published version, for example 2.2.0")
    parser.add_argument("--output", required=True, help="path for the verified installer")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL,
                        help="directory that serves the published downloads")
    parser.add_argument("--attempts", type=int, default=20,
                        help="download attempts while Pages propagates a deployment")
    parser.add_argument("--delay", type=float, default=15.0,
                        help="seconds between attempts")
    parser.add_argument("--timeout", type=float, default=60.0,
                        help="per-request timeout in seconds")
    args = parser.parse_args()

    name = f"NEXUS-Setup-{args.version}.exe"
    base = args.base_url.rstrip("/")
    output = Path(args.output)
    staging = output.with_name(output.name + ".download")
    checksum = output.with_name(output.name + ".sha256.txt")

    attempts = max(args.attempts, 1)
    failure_kind = 4
    last_error = "no attempt was made"
    for attempt in range(1, attempts + 1):
        try:
            download(f"{base}/{name}", staging, args.timeout)
        except (urllib.error.URLError, OSError) as error:
            failure_kind = 4
            last_error = f"download failed: {error}"
        else:
            problem = pe_problem(staging.read_bytes(), name)
            if problem is not None:
                failure_kind = 2
                last_error = problem
            else:
                try:
                    download(f"{base}/{name}.sha256.txt", checksum, args.timeout)
                    problem = verify_checksum(staging, checksum, name)
                except (urllib.error.URLError, OSError) as error:
                    problem = f"checksum download failed: {error}"
                if problem is None:
                    staging.replace(output)
                    checksum.unlink(missing_ok=True)
                    print(f"PASS downloaded and verified {name}")
                    return 0
                failure_kind = 3
                last_error = problem
        staging.unlink(missing_ok=True)
        checksum.unlink(missing_ok=True)
        if attempt < attempts:
            print(f"attempt {attempt}/{attempts}: {last_error}; "
                  f"retrying in {args.delay:g}s", file=sys.stderr)
            if args.delay > 0:
                time.sleep(args.delay)

    print(f"ERROR: could not obtain {name} from {base} after {attempts} attempt(s): "
          f"{last_error}", file=sys.stderr)
    return failure_kind


if __name__ == "__main__":
    raise SystemExit(main())
