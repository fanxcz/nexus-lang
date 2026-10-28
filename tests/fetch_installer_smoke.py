#!/usr/bin/env python3
"""Regression coverage for scripts/fetch_installer.py.

The Poker CI job used to fail because GitHub Pages answered a request for an
installer that had not been deployed yet with an HTML error page, which the
workflow stored as NEXUS-Setup.exe. These tests pin the required behaviour:
an HTML payload is rejected with a clear message, a checksum mismatch is
reported separately, and a verified PE image is accepted.
"""
from __future__ import annotations

import hashlib
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent.parent / "scripts" / "fetch_installer.py"
VERSION = "9.9.9"
NAME = f"NEXUS-Setup-{VERSION}.exe"
failures = 0


def run(base: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), "--version", VERSION,
         "--base-url", base.as_uri(), "--output", str(base / NAME),
         "--attempts", "1", "--delay", "0", "--timeout", "10"],
        capture_output=True, text=True, check=False)


def check(label: str, condition: bool, detail: str = "") -> None:
    global failures
    if not condition:
        failures += 1
        print(f"fetch_installer smoke: {label} FAILED {detail}".rstrip(), file=sys.stderr)


def fake_pe() -> bytes:
    data = bytearray(256)
    data[0:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 64)
    data[64:68] = b"PE\0\0"
    return bytes(data)


def write_checksum(base: Path, payload: bytes) -> None:
    digest = hashlib.sha256(payload).hexdigest()
    (base / f"{NAME}.sha256.txt").write_text(f"{digest}  {NAME}\n", encoding="ascii")


def main() -> int:
    with tempfile.TemporaryDirectory() as workspace:
        base = Path(workspace)

        # Missing download: the Pages deployment has not produced the file yet.
        result = run(base)
        check("missing download", result.returncode == 4, f"rc={result.returncode} {result.stderr}")
        check("missing download message", "could not obtain" in result.stderr, result.stderr)

        # GitHub Pages 404 page stored under the installer name.
        (base / NAME).write_text(
            "<!DOCTYPE html><html><head><title>404</title></head>"
            "<body>404 - Not found</body></html>\n", encoding="utf-8")
        result = run(base)
        check("html payload", result.returncode == 2, f"rc={result.returncode} {result.stderr}")
        check("html payload message",
              "not a Windows PE executable" in result.stderr and "HTML" in result.stderr,
              result.stderr)

        # A real PE image whose checksum file was replaced by a Pages 404 page.
        payload = fake_pe()
        (base / NAME).write_bytes(payload)
        (base / f"{NAME}.sha256.txt").write_text("<!DOCTYPE html><html>404</html>\n",
                                                 encoding="utf-8")
        result = run(base)
        check("html checksum", result.returncode == 3, f"rc={result.returncode} {result.stderr}")

        # A real PE image with a checksum that belongs to different bytes.
        (base / f"{NAME}.sha256.txt").write_text("0" * 64 + f"  {NAME}\n", encoding="ascii")
        result = run(base)
        check("checksum mismatch", result.returncode == 3, f"rc={result.returncode} {result.stderr}")

        # A PE image whose published checksum matches must be accepted.
        write_checksum(base, payload)
        result = run(base)
        check("verified installer", result.returncode == 0, f"rc={result.returncode} {result.stderr}")
        check("verified installer output", (base / NAME).read_bytes() == payload)
        check("temporary checksum removed", not (base / f"{NAME}.sha256.txt").exists())
        check("no partial download left", not (base / f"{NAME}.download").exists())

    if failures:
        return 1
    print("NEXUS fetch_installer smoke: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
