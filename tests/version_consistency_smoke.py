#!/usr/bin/env python3
"""Regression test for scripts/nexus_version.py.

The checker is what keeps the workflows, installer and download lists on one
version. This test proves it accepts an internally consistent tree and rejects
the failures it exists for: a bumped compiler version that leaves older
artifact names or installer pins behind, and an installer whose output name no
longer derives from the version macro.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
COMPILER = "9.9.9"
EXTENSION = COMPILER
STUDIO = COMPILER


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def project_file(version: str) -> str:
    return (
        "cmake_minimum_required(VERSION 3.20)\n"
        f"project(nexus VERSION {version} LANGUAGES CXX C)\n"
    )


def build_fixture(root: Path) -> None:
    shutil.copytree(
        REPO / "scripts",
        root / "scripts",
        ignore=shutil.ignore_patterns("__pycache__", "*.pyc"),
    )
    write(root / "CMakeLists.txt", project_file(COMPILER))
    write(
        root / ".github/project-tools/vscode-extension/package.json",
        json.dumps({"name": "nexus-language", "version": EXTENSION}),
    )
    write(
        root / ".github/project-tools/nexus-studio/package.json",
        json.dumps({"name": "nexus-studio", "version": STUDIO}),
    )
    write(
        root / ".github/project-tools/installer/NEXUS-Setup.iss",
        "#define MyAppName \"NEXUS\"\n"
        "#ifndef MyAppVersion\n"
        f"  #define MyAppVersion \"{COMPILER}\"\n"
        "#endif\n"
        "[Setup]\n"
        "OutputBaseFilename=NEXUS-Setup-{#MyAppVersion}\n"
        "VersionInfoVersion={#MyAppVersion}.0\n"
        "Parameters := '-Mode Check -ExpectedVersion \"{#MyAppVersion}\"';\n"
        "NewPath := AppPath;\n",
    )
    write(
        root / ".github/project-tools/repository-files/NEXUS-Windows.ps1",
        "param([string]$ExpectedVersion = '')\n",
    )
    write(
        root / ".github/project-tools/installer/NEXUS-Paint-Setup.iss",
        f'#define MyAppVersion "{COMPILER}"\n'
        f'VersionInfoVersion={COMPILER}.0\n'
        f'VersionInfoProductVersion={COMPILER}\n'
        f'UninstallDisplayName=NEXUS Paint {COMPILER}\n',
    )
    write(
        root / "Install-NEXUS.ps1",
        f"$version = '{COMPILER}'\n$installedLauncher = 'nexus.cmd'\n$versionPattern = '{COMPILER}'\n",
    )
    write(
        root / ".github/project-tools/repository-files/Install-NEXUS-Path.ps1",
        "$NewPath = (@($Root) + $OtherEntries) -join ';'\n",
    )
    write(
        root / "Install-NEXUS.bat",
        f"title NEXUS {COMPILER} Installer\necho NEXUS Setup {COMPILER}\n",
    )
    write(
        root / ".github/project-tools/repository-files/NEXUS-Console.bat",
        f"title NEXUS {COMPILER} Console\n",
    )
    write(
        root / ".github/project-tools/repository-files/THIRD_PARTY_NOTICES.md",
        f"# NEXUS third-party notices (v{COMPILER}).\n",
    )
    write(
        root / "docs/downloads.md",
        f"[Setup](assets/downloads/NEXUS-Setup-{COMPILER}.exe)\n"
        f"[Extension](assets/downloads/NEXUS-Language-{EXTENSION}.vsix)\n"
        f"[Studio](https://github.com/fanxcz/nexus-lang/releases/download/studio-v{STUDIO}/NEXUS-Studio-{STUDIO}-Windows-x64.exe)\n",
    )
    write(root / "docs/manual.md", f"# NEXUS Language Manual\n\nVersion {COMPILER}\n")


def run(script: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *arguments],
        capture_output=True,
        text=True,
    )


def expect_ok(script: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    result = run(script, *arguments)
    if result.returncode != 0:
        raise AssertionError(
            f"{' '.join(arguments)} failed with {result.returncode}:\n"
            f"{result.stdout}\n{result.stderr}"
        )
    return result


def expect_rejected(script: Path, *arguments: str, contains: tuple[str, ...]) -> None:
    result = run(script, *arguments)
    if result.returncode == 0:
        raise AssertionError(f"{' '.join(arguments)} should have failed:\n{result.stdout}")
    for needle in contains:
        if needle not in result.stderr:
            raise AssertionError(
                f"expected {needle!r} in the failure output:\n{result.stderr}"
            )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nexus", type=Path, required=True, help="path to the built compiler")
    args = parser.parse_args()

    repo_script = REPO / "scripts/nexus_version.py"
    expect_ok(repo_script, "check", "--root", str(REPO), "--nexus-bin", str(args.nexus))
    compiler = re.search(
        r"project\s*\(\s*nexus\s+VERSION\s+(\d+\.\d+\.\d+)",
        (REPO / "CMakeLists.txt").read_text(encoding="utf-8"),
    ).group(1)
    extension = json.loads(
        (REPO / ".github/project-tools/vscode-extension/package.json").read_text(encoding="utf-8")
    )["version"]
    for what, expected in (("compiler", compiler), ("extension", extension), ("studio", extension)):
        result = expect_ok(repo_script, what)
        if result.stdout.strip() != expected:
            raise AssertionError(f"{what} printed {result.stdout.strip()!r}, expected {expected!r}")
    env = expect_ok(repo_script, "github-env").stdout
    for line in (f"NEXUS_VERSION={compiler}",
                 f"NEXUS_EXTENSION_VERSION={extension}",
                 f"NEXUS_STUDIO_VERSION={extension}",
                 f"NEXUS_SETUP_ASSET=NEXUS-Setup-{compiler}.exe",
                 f"NEXUS_VSIX_ASSET=NEXUS-Language-{extension}.vsix"):
        if line not in env:
            raise AssertionError(f"github-env output is missing {line!r}:\n{env}")

    with tempfile.TemporaryDirectory(prefix="nexus-version-") as directory:
        root = Path(directory)
        build_fixture(root)
        script = root / "scripts/nexus_version.py"
        expect_ok(script, "check", "--root", str(root))

        write(
            root / ".github/project-tools/nexus-studio/package.json",
            json.dumps({"name": "nexus-studio", "version": "9.9.10"}),
        )
        expect_rejected(
            script, "check", "--root", str(root),
            contains=("Studio", f"NEXUS-Studio-{STUDIO}-Windows-x64.exe"),
        )
        write(
            root / ".github/project-tools/nexus-studio/package.json",
            json.dumps({"name": "nexus-studio", "version": STUDIO}),
        )

        write(root / "CMakeLists.txt", project_file("9.9.10"))
        expect_rejected(
            script, "check", "--root", str(root),
            contains=("Install-NEXUS.ps1", "downloads.md", "NEXUS-Setup.iss", "NEXUS-Paint-Setup.iss", "9.9.10"),
        )

        write(root / "CMakeLists.txt", project_file(COMPILER))
        write(
            root / ".github/project-tools/repository-files/NEXUS-Windows.ps1",
            "param([string]$OtherValue = '')\n",
        )
        expect_rejected(
            script, "check", "--root", str(root),
            contains=("NEXUS-Windows.ps1", "ExpectedVersion"),
        )
        write(
            root / ".github/project-tools/repository-files/NEXUS-Windows.ps1",
        "param([string]$ExpectedVersion = '')\n",
        )

        write(
            root / ".github/project-tools/installer/NEXUS-Setup.iss",
            "[Setup]\nOutputBaseFilename=NEXUS-Setup-9.9.9\n",
        )
        expect_rejected(
            script, "check", "--root", str(root),
            contains=("OutputBaseFilename",),
        )

    print("NEXUS version consistency smoke test: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
