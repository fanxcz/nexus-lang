#!/usr/bin/env python3
"""Repository-wide integrity audit for tracked NEXUS files.

This is a fast preflight, not a substitute for the compiler tests, sanitizer jobs,
documentation build, or platform-specific packaging checks.
"""
from __future__ import annotations

import ast
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEXT_SUFFIXES = {
    ".bat", ".c", ".cmake", ".cmd", ".cpp", ".css", ".cjs", ".h", ".hpp",
    ".html", ".iss", ".js", ".json", ".md", ".mjs", ".nx", ".ps1", ".py",
    ".sh", ".svg", ".toml", ".trigger", ".txt", ".yml", ".yaml",
}
TEXT_NAMES = {
    ".clang-format", ".editorconfig", ".gitattributes", ".gitignore",
    ".vscodeignore", "LICENSE",
}
CONFLICT_START = re.compile(r"^<<<<<<< (?:HEAD|.+)$")
CONFLICT_END = re.compile(r"^>>>>>>> .+$")


def fail(message: str) -> None:
    print(f"FAIL repository audit: {message}", file=sys.stderr)


def tracked_paths() -> list[str]:
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    return [item.decode("utf-8", errors="strict") for item in result.stdout.split(b"\0") if item]


def check_conflict_markers(path: str, text: str) -> list[str]:
    errors: list[str] = []
    start_line: int | None = None
    has_separator = False
    for line_number, line in enumerate(text.splitlines(), start=1):
        if start_line is None and CONFLICT_START.match(line):
            start_line = line_number
            has_separator = False
        elif start_line is not None and line == "=======":
            has_separator = True
        elif start_line is not None and CONFLICT_END.match(line):
            if has_separator:
                errors.append(f"{path}:{start_line}: unresolved merge-conflict markers")
            start_line = None
            has_separator = False
    if start_line is not None:
        errors.append(f"{path}:{start_line}: incomplete merge-conflict marker block")
    return errors


def check_css(path: str, text: str) -> list[str]:
    # Remove comments and quoted strings before checking block braces. Full CSS
    # parsing remains the responsibility of the site/desktop build tooling.
    source = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    source = re.sub(r'"(?:\\\\.|[^"\\\\])*"|\'(?:\\\\.|[^\'\\\\])*\'', '""', source)
    depth = 0
    for offset, char in enumerate(source):
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth < 0:
                return [f"{path}: closing brace without matching opening brace near offset {offset}"]
    if depth:
        return [f"{path}: {depth} CSS block brace(s) left unclosed"]
    return []


def main() -> int:
    try:
        paths = tracked_paths()
    except (OSError, subprocess.CalledProcessError, UnicodeDecodeError) as exc:
        fail(f"could not enumerate tracked Git files: {exc}")
        return 1

    if not paths:
        fail("Git returned no tracked files; run this script from a repository checkout")
        return 1

    errors: list[str] = []
    text_count = 0
    json_count = 0
    python_count = 0
    js_paths: list[str] = []
    css_count = 0
    total_bytes = 0

    for relative in paths:
        path = ROOT / relative
        if not path.exists() and not path.is_symlink():
            errors.append(f"{relative}: tracked file is missing from the checkout")
            continue
        if path.is_symlink():
            try:
                resolved = path.resolve(strict=True)
                resolved.relative_to(ROOT)
            except (OSError, ValueError):
                errors.append(f"{relative}: broken symlink or symlink escapes the repository")
            continue
        if not path.is_file():
            errors.append(f"{relative}: tracked path is not a regular file")
            continue

        try:
            total_bytes += path.stat().st_size
            payload = path.read_bytes()
        except OSError as exc:
            errors.append(f"{relative}: could not read file ({exc})")
            continue

        suffix = path.suffix.lower()
        is_text = suffix in TEXT_SUFFIXES or path.name in TEXT_NAMES or relative.endswith(".json.example")
        if not is_text:
            continue
        text_count += 1

        if b"\0" in payload:
            errors.append(f"{relative}: NUL byte found in a text file")
            continue
        try:
            contents = payload.decode("utf-8")
        except UnicodeDecodeError as exc:
            errors.append(f"{relative}: text is not valid UTF-8 ({exc})")
            continue

        errors.extend(check_conflict_markers(relative, contents))

        if suffix == ".json" or relative.endswith(".json.example"):
            json_count += 1
            try:
                json.loads(contents)
            except json.JSONDecodeError as exc:
                errors.append(f"{relative}:{exc.lineno}:{exc.colno}: invalid JSON ({exc.msg})")

        if suffix == ".py":
            python_count += 1
            try:
                ast.parse(contents, filename=relative)
            except SyntaxError as exc:
                errors.append(f"{relative}:{exc.lineno}:{exc.offset}: invalid Python syntax ({exc.msg})")

        if suffix in {".js", ".cjs", ".mjs"}:
            js_paths.append(relative)

        if suffix == ".css":
            css_count += 1
            errors.extend(check_css(relative, contents))

    node = shutil.which("node")
    if js_paths and not node:
        errors.append("Node.js is required to syntax-check tracked JavaScript files")
    elif node:
        for relative in js_paths:
            result = subprocess.run(
                [node, "--check", str(ROOT / relative)],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if result.returncode:
                detail = (result.stderr or result.stdout).strip().splitlines()
                errors.append(f"{relative}: Node.js syntax check failed: {detail[-1] if detail else 'unknown error'}")

    if errors:
        for error in errors:
            print(f"FAIL {error}", file=sys.stderr)
        print(
            f"Repository audit failed: {len(errors)} issue(s) across {len(paths)} tracked files.",
            file=sys.stderr,
        )
        return 1

    print(
        "PASS repository audit: "
        f"{len(paths)} tracked files inventoried, {text_count} UTF-8 text files checked, "
        f"{json_count} JSON documents parsed, {python_count} Python files parsed, "
        f"{len(js_paths)} JavaScript files syntax-checked, {css_count} stylesheets checked; "
        f"{total_bytes:,} tracked bytes inventoried."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
