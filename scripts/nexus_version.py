#!/usr/bin/env python3
"""Single source of truth for the NEXUS version numbers used by the repository.

Two versions exist and are intentionally independent:

* the compiler/installer version comes from ``project(nexus VERSION x.y.z)`` in
  ``CMakeLists.txt``;
* the VS Code extension version comes from
  ``.github/project-tools/vscode-extension/package.json``.

Workflows, packaging scripts and the download validators ask this module instead
of copying the numbers. ``check`` verifies that no tracked text file still pins a
stale artifact name and that the installer, bootstrap scripts and download
lists agree with the two sources above. It is wired into CTest, so a version
bump that misses a file fails locally and in CI.

Usage::

    python scripts/nexus_version.py compiler
    python scripts/nexus_version.py extension
    python scripts/nexus_version.py setup        # NEXUS-Setup-x.y.z.exe
    python scripts/nexus_version.py vsix         # NEXUS-Language-x.y.z.vsix
    python scripts/nexus_version.py github-env   # lines for $GITHUB_ENV
    python scripts/nexus_version.py check [--root DIR] [--nexus-bin PATH]
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROJECT_FILE = "CMakeLists.txt"
EXTENSION_PACKAGE = Path(".github/project-tools/vscode-extension/package.json")
SETUP_INSTALLER = Path(".github/project-tools/installer/NEXUS-Setup.iss")
PORTABLE_ARCHIVE = "NEXUS_Windows_Portable.zip"
PAINT_EXECUTABLE = "NEXUS-Paint-Windows-x64.exe"
PAINT_ARCHIVE = "NEXUS-Paint-Windows-x64.zip"

VERSION_PATTERN = r"(\d+\.\d+\.\d+)"
PROJECT_VERSION = re.compile(
    r"^\s*project\s*\(\s*nexus\s+VERSION\s+" + VERSION_PATTERN, re.M
)
SETUP_ASSET = re.compile(r"NEXUS-Setup-" + VERSION_PATTERN + r"\.exe")
VSIX_ASSET = re.compile(r"NEXUS-Language-" + VERSION_PATTERN + r"\.vsix")

SKIP_DIRS = {
    ".git", "build", "build-verify", "site", "dist", "node_modules", "target", ".venv",
    "__pycache__",
}
BINARY_SUFFIXES = {
    ".exe", ".zip", ".vsix", ".dll", ".png", ".jpg", ".jpeg", ".bmp", ".wav", ".obj",
    ".pdf", ".ico", ".ttf", ".woff", ".woff2", ".pyc",
}
# Release notes describe past releases on purpose; only current-state files are
# expected to name the shipped artifacts.
HISTORICAL_NAMES = {"changelog.md"}


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig", errors="replace")


def compiler_version(root: Path = ROOT) -> str:
    """Return the compiler/installer version from ``CMakeLists.txt``."""
    match = PROJECT_VERSION.search(_read(root / PROJECT_FILE))
    if not match:
        raise SystemExit(f"could not read the project version from {root / PROJECT_FILE}")
    return match.group(1)


def extension_version(root: Path = ROOT) -> str:
    """Return the VS Code extension version from its ``package.json``."""
    package = root / EXTENSION_PACKAGE
    try:
        data = json.loads(_read(package))
        version = data["version"]
    except (OSError, ValueError, KeyError) as error:
        raise SystemExit(f"could not read the extension version from {package}: {error}")
    if not re.fullmatch(VERSION_PATTERN, str(version)):
        raise SystemExit(f"{package}: version {version!r} is not x.y.z")
    return str(version)


def asset_names(root: Path = ROOT) -> dict[str, str]:
    """Return every download artifact name, keyed by a short stable label."""
    compiler = compiler_version(root)
    extension = extension_version(root)
    return {
        "setup": f"NEXUS-Setup-{compiler}.exe",
        "portable": PORTABLE_ARCHIVE,
        "vsix": f"NEXUS-Language-{extension}.vsix",
        "paint_executable": PAINT_EXECUTABLE,
        "paint_archive": PAINT_ARCHIVE,
    }


def _text_files(root: Path):
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        if any(part in SKIP_DIRS for part in path.relative_to(root).parts[:-1]):
            continue
        if path.name.lower() in HISTORICAL_NAMES:
            continue
        if path.suffix.lower() in BINARY_SUFFIXES:
            continue
        yield path


def _pinned_version(path: Path, pattern: str) -> str | None:
    match = re.search(pattern, _read(path), re.M)
    return match.group(1) if match else None


def check(root: Path = ROOT, nexus_bin: Path | None = None) -> list[str]:
    """Return one message per inconsistency; an empty list means everything agrees."""
    compiler = compiler_version(root)
    extension = extension_version(root)
    problems: list[str] = []

    for path in _text_files(root):
        text = _read(path)
        for match in SETUP_ASSET.finditer(text):
            if match.group(1) != compiler:
                problems.append(
                    f"{path.relative_to(root)}: names NEXUS-Setup-{match.group(1)}.exe, "
                    f"but the compiler version is {compiler}"
                )
        for match in VSIX_ASSET.finditer(text):
            if match.group(1) != extension:
                problems.append(
                    f"{path.relative_to(root)}: names NEXUS-Language-{match.group(1)}.vsix, "
                    f"but the extension version is {extension}"
                )

    pins = (
        (SETUP_INSTALLER, r'#define\s+MyAppVersion\s+"(\d+\.\d+\.\d+)"'),
        (Path("Install-NEXUS.ps1"), r"\$version\s*=\s*'(\d+\.\d+\.\d+)'"),
        (Path("Install-NEXUS.bat"), r"title\s+NEXUS\s+(\d+\.\d+\.\d+)\s+Installer"),
        (Path("Install-NEXUS.bat"), r"echo\s+NEXUS\s+Setup\s+(\d+\.\d+\.\d+)"),
        (Path(".github/project-tools/repository-files/NEXUS-Console.bat"),
         r"title\s+NEXUS\s+(\d+\.\d+\.\d+)\s+Console"),
        (Path(".github/project-tools/repository-files/THIRD_PARTY_NOTICES.md"),
         r"\(v(\d+\.\d+\.\d+)\)"),
        (Path("docs/manual.md"), r"^Version\s+(\d+\.\d+\.\d+)\s*$"),
    )
    for relative, pattern in pins:
        path = root / relative
        if not path.is_file():
            problems.append(f"{relative}: file is missing")
            continue
        pinned = _pinned_version(path, pattern)
        if pinned is None:
            problems.append(f"{relative}: could not find a version pin matching {pattern!r}")
        elif pinned != compiler:
            problems.append(f"{relative}: pins {pinned}, but the compiler version is {compiler}")

    setup = root / SETUP_INSTALLER
    if setup.is_file() and "NEXUS-Setup-{#MyAppVersion}" not in _read(setup):
        problems.append(
            f"{SETUP_INSTALLER}: OutputBaseFilename must be NEXUS-Setup-{{#MyAppVersion}}"
        )

    sys.path.insert(0, str(root / "scripts"))
    try:
        import check_repository_links
        import validate_downloads
    except ImportError as error:  # pragma: no cover - defensive
        problems.append(f"scripts package is not importable: {error}")
    else:
        expected = asset_names(root)
        expected_set = set(expected.values())
        expected_set.update(name + ".sha256.txt" for name in list(expected_set))
        if set(validate_downloads.ARTIFACTS) != set(expected.values()):
            problems.append(
                "scripts/validate_downloads.py: ARTIFACTS disagrees with the version sources"
            )
        if set(check_repository_links.GENERATED_ASSETS) != expected_set:
            problems.append(
                "scripts/check_repository_links.py: GENERATED_ASSETS disagrees with "
                "the version sources"
            )

    if nexus_bin is not None:
        try:
            output = subprocess.run(
                [str(nexus_bin), "--version"],
                check=True,
                capture_output=True,
                text=True,
            ).stdout
        except (OSError, subprocess.CalledProcessError) as error:
            problems.append(f"{nexus_bin} --version failed: {error}")
        else:
            if not re.search(r"Nexus\s+" + re.escape(compiler) + r"\b", output):
                problems.append(
                    f"{nexus_bin} --version does not report {compiler}: {output.strip()!r}"
                )

    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "what",
        choices=["compiler", "extension", "setup", "vsix", "github-env", "check"],
        help="value to print, or `check` to verify the repository",
    )
    parser.add_argument("--root", type=Path, default=ROOT, help="repository root")
    parser.add_argument("--nexus-bin", type=Path, default=None,
                        help="also verify that this compiler reports the version")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    if args.what == "compiler":
        print(compiler_version(root))
    elif args.what == "extension":
        print(extension_version(root))
    elif args.what == "setup":
        print(asset_names(root)["setup"])
    elif args.what == "vsix":
        print(asset_names(root)["vsix"])
    elif args.what == "github-env":
        compiler = compiler_version(root)
        extension = extension_version(root)
        print(f"NEXUS_VERSION={compiler}")
        print(f"NEXUS_EXTENSION_VERSION={extension}")
        print(f"NEXUS_SETUP_ASSET=NEXUS-Setup-{compiler}.exe")
        print(f"NEXUS_VSIX_ASSET=NEXUS-Language-{extension}.vsix")
    else:
        problems = check(root, args.nexus_bin)
        if problems:
            for problem in problems:
                print(f"FAIL {problem}", file=sys.stderr)
            return 1
        print(
            "PASS version consistency: "
            f"compiler {compiler_version(root)}, extension {extension_version(root)}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
