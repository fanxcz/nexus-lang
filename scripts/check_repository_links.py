#!/usr/bin/env python3
"""Validate local links and same-repository URLs throughout NEXUS text files."""
from __future__ import annotations
import os
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlparse

ROOT = Path.cwd().resolve()
TEXT_SUFFIXES = {
    ".md", ".mdx", ".html", ".htm", ".txt", ".yml", ".yaml", ".json",
    ".js", ".cjs", ".mjs", ".css", ".scss", ".nx", ".cpp", ".hpp",
    ".h", ".c", ".cmake", ".sh", ".ps1", ".bat", ".cmd", ".iss",
    ".toml", ".ini", ".xml", ".svg", ".example",
}
SKIP_DIRS = {".git", "node_modules", "build", "site", "dist", "target", ".venv"}
BINARY_SUFFIXES = {".exe", ".zip", ".bmp", ".wav", ".obj", ".png", ".jpg", ".jpeg", ".dll", ".vsix"}
GENERATED_ASSETS = {
    "NEXUS-Setup-2.1.1.exe", "NEXUS-Setup-2.1.1.exe.sha256.txt",
    "NEXUS_Windows_Portable.zip", "NEXUS_Windows_Portable.zip.sha256.txt",
    "NEXUS-Language-2.1.0.vsix", "NEXUS-Language-2.1.0.vsix.sha256.txt",
    "NEXUS-Paint-Windows-x64.exe", "NEXUS-Paint-Windows-x64.exe.sha256.txt",
    "NEXUS-Paint-Windows-x64.zip", "NEXUS-Paint-Windows-x64.zip.sha256.txt",
}
MD_LINK = re.compile(r"!?\[[^\]]*\]\(\s*(?:<([^>]+)>|([^\s)]+))", re.M)
MD_REF = re.compile(r"(?m)^\s{0,3}\[[^\]]+\]:\s*<?([^\s>]+)>?")
HTML_ATTR = re.compile(r"""(?i)\b(?:href|src|poster|action)\s*=\s*["']([^"']+)["']""")
CSS_URL = re.compile(r"""(?i)\burl\(\s*["']?([^)'"\s]+)""")
BARE_URL = re.compile(r"""https?://[^\s"'<>\x60]+""")
HEADING = re.compile(r"(?m)^\s{0,3}#{1,6}\s+(.+?)\s*#*\s*$")
HTML_ID = re.compile(r"""(?i)\b(?:id|name)\s*=\s*["']([^"']+)["']""")
GITHUB_FILE = re.compile(
    r"^https://(?:raw\.githubusercontent\.com|github\.com)/fanxcz/nexus-lang/"
    r"(?:blob/|tree/|raw/)?main/([^?#]+)", re.I
)
LEGACY_URLS = (
    re.compile(r"(?i)https?://(?:www\.)?github\.com/fanxcz/nexus(?:/|\.git(?:[?#]|$)|[?#]|$)"),
    re.compile(r"(?i)https?://fanxcz\.github\.io/nexus(?:/|[?#]|$)"),
)

def source_files() -> list[Path]:
    result: list[Path] = []
    for base, dirs, names in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS and not d.startswith(".git")]
        for name in names:
            p = Path(base, name)
            if p.suffix.lower() in BINARY_SUFFIXES:
                continue
            if p.suffix.lower() in TEXT_SUFFIXES or name in {".gitignore", ".gitattributes"}:
                result.append(p)
    return sorted(set(result))

def heading_slug(value: str) -> str:
    tick = chr(96)
    value = re.sub(tick + r"([^" + tick + r"]*)" + tick, r"\1", value)
    value = re.sub(r"\[([^\]]+)\]\([^)]*\)", r"\1", value)
    value = re.sub(r"<[^>]+>", "", value).lower().strip()
    value = re.sub(r"[^\w\- ]", "", value, flags=re.UNICODE)
    return re.sub(r"\s+", "-", value)

def anchors_for(path: Path) -> set[str]:
    try:
        content = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return set()
    anchors: set[str] = set()
    counts: dict[str, int] = {}
    for title in HEADING.findall(content):
        base = heading_slug(title)
        number = counts.get(base, 0)
        counts[base] = number + 1
        anchors.add(base if number == 0 else f"{base}-{number}")
    anchors.update(HTML_ID.findall(content))
    return anchors

def resolve_local(source: Path, target: str) -> Path | None:
    target = unquote(target.strip())
    if not target or target.startswith("#"):
        return source
    if target.startswith("/"):
        candidate = ROOT / target.lstrip("/")
    else:
        candidate = source.parent / target
    try:
        resolved = candidate.resolve(strict=False)
        resolved.relative_to(ROOT)
    except (OSError, ValueError):
        return None
    candidates = [resolved]
    if not resolved.suffix:
        candidates.extend([resolved.with_suffix(".md"), resolved / "index.md", resolved / "README.md"])
    for item in candidates:
        if item.is_file():
            return item
    # The download files are deliberately created by the packaging workflow,
    # not committed under docs/assets/downloads.
    if resolved.parent == (ROOT / "docs/assets/downloads").resolve() and resolved.name in GENERATED_ASSETS:
        return resolved
    return None

def repo_url_path(url: str) -> tuple[bool, str] | None:
    match = GITHUB_FILE.match(url)
    if not match:
        return None
    path = unquote(match.group(1)).rstrip("/")
    candidate = ROOT / path
    return (candidate.is_file() or candidate.is_dir(), path)

def strip_markdown_code(content: str) -> str:
    # Code samples often contain C++ lambda syntax like [](const X&){} that
    # resembles a Markdown link. Ignore code when parsing navigation links;
    # bare URL extraction below still sees all text in the original file.
    lines = content.splitlines()
    output: list[str] = []
    fence = None
    tick = chr(96)
    for line in lines:
        stripped = line.lstrip()
        marker = tick * 3 if stripped.startswith(tick * 3) else ("~~~" if stripped.startswith("~~~") else None)
        if fence is None and marker:
            fence = marker
            output.append("")
            continue
        if fence is not None:
            output.append("")
            if stripped.startswith(fence):
                fence = None
            continue
        output.append(line)
    text = "\n".join(output)
    return re.sub(tick + r"+[^" + tick + r"\n]*" + tick + r"+", "", text)

def main() -> int:
    files = source_files()
    errors: list[str] = []
    checked = 0
    urls = 0
    for source in files:
        try:
            content = source.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        rel = source.relative_to(ROOT)
        targets: list[tuple[str, int]] = []
        suffix = source.suffix.lower()
        parse_content = strip_markdown_code(content) if suffix in {".md", ".mdx", ".rst", ".txt"} else content
        patterns = []
        if suffix in {".md", ".mdx", ".rst", ".txt"}:
            patterns.extend((MD_LINK, MD_REF))
        if suffix in {".md", ".mdx", ".html", ".htm", ".svg"}:
            patterns.append(HTML_ATTR)
        if suffix in {".html", ".htm", ".css", ".scss", ".svg"}:
            patterns.append(CSS_URL)
        for pattern in patterns:
            for match in pattern.finditer(parse_content):
                target = next((group for group in match.groups() if group is not None), "").strip()
                if target:
                    targets.append((target, parse_content.count("\n", 0, match.start()) + 1))
        for match in BARE_URL.finditer(content):
            raw = match.group(0)
            url = raw.rstrip(".,;:!?)]}{")
            urls += 1
            line = content.count("\n", 0, match.start()) + 1
            if any(pattern.search(url) for pattern in LEGACY_URLS):
                errors.append(
                    f"{rel}:{line}: obsolete repository/site URL; use "
                    f"https://github.com/fanxcz/nexus-lang or "
                    f"https://fanxcz.github.io/nexus-lang/: {url}"
                )
            own = repo_url_path(url)
            if own and not own[0]:
                offset = content.find(raw)
                line = content.count("\n", 0, max(offset, 0)) + 1
                errors.append(f"{rel}:{line}: repository URL points to missing path: {url}")
        for target, line in targets:
            if target.startswith(("https://", "http://")):
                own = repo_url_path(target)
                if own and not own[0]:
                    errors.append(f"{rel}:{line}: repository URL points to missing path: {target}")
                continue
            if target.startswith(("mailto:", "tel:", "javascript:", "data:", "vscode:", "command:")):
                continue
            if target.startswith("{") or "{{" in target or "}}" in target:
                continue
            if target.startswith("//"):
                continue
            parsed = urlparse(target)
            if parsed.scheme:
                continue
            path_part = target.split("#", 1)[0].split("?", 1)[0]
            fragment = target.split("#", 1)[1].split("?", 1)[0] if "#" in target else ""
            target_file = source if not path_part else resolve_local(source, path_part)
            if target_file is None:
                errors.append(f"{rel}:{line}: broken local link: {target}")
                continue
            checked += 1
            if fragment and target_file.suffix.lower() == ".md":
                anchors = anchors_for(target_file)
                if fragment not in anchors and unquote(fragment) not in anchors:
                    errors.append(f"{rel}:{line}: missing heading anchor #{fragment} in {target_file.relative_to(ROOT)}")
    print(f"Scanned {len(files)} text files; checked {checked} local link(s); found {urls} URL occurrence(s).")
    if errors:
        print("\nLINK ERRORS:")
        for issue in errors:
            print(" - " + issue)
        print(f"\nFAILED: {len(errors)} link problem(s).")
        return 1
    print("PASS: no broken local links or same-repository GitHub file URLs found.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
