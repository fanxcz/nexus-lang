#!/usr/bin/env python3
"""Assemble and validate the hand-authored NEXUS HTML website.

The website source is plain HTML, CSS, JavaScript and SVG under website/.
This script copies those files as-is, adds verified release assets, and checks
all local HTML links. It deliberately does not render Markdown or use a theme,
template engine, static-site framework, or frontend build system.
"""
from __future__ import annotations

import argparse
import shutil
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

from nexus_version import ROOT, asset_names

REQUIRED_PAGES = {
    "index.html",
    "start.html",
    "language.html",
    "runtime.html",
    "tooling.html",
    "applications.html",
    "studio.html",
    "downloads.html",
    "reference.html",
}


class PageAudit(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.ids: set[str] = set()
        self.references: list[tuple[str, str]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        element_id = values.get("id")
        if element_id:
            self.ids.add(element_id)
        for attribute in ("href", "src", "poster"):
            value = values.get(attribute)
            if value:
                self.references.append((attribute, value))

    def handle_startendtag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self.handle_starttag(tag, attrs)


def audit_site(site: Path) -> list[str]:
    errors: list[str] = []
    pages = sorted(site.rglob("*.html"))
    known_ids: dict[Path, set[str]] = {}

    for page in pages:
        parser = PageAudit()
        try:
            parser.feed(page.read_text(encoding="utf-8"))
            parser.close()
        except (OSError, UnicodeError) as error:
            errors.append(f"{page.relative_to(site)}: could not read HTML: {error}")
            continue
        known_ids[page.resolve()] = parser.ids

    for page in pages:
        parser = PageAudit()
        parser.feed(page.read_text(encoding="utf-8"))
        parser.close()
        for attribute, reference in parser.references:
            parsed = urlsplit(reference)
            if parsed.scheme or parsed.netloc or reference.startswith("//"):
                continue
            if not parsed.path:
                if parsed.fragment and parsed.fragment not in known_ids.get(page.resolve(), set()):
                    errors.append(f"{page.relative_to(site)}: missing anchor #{parsed.fragment}")
                continue

            relative = Path(unquote(parsed.path.lstrip("/")))
            target = (site / relative) if parsed.path.startswith("/") else (page.parent / relative)
            target = target.resolve()
            if target.is_dir():
                target = target / "index.html"
            if not target.is_file():
                errors.append(
                    f"{page.relative_to(site)}: broken {attribute}={reference!r}"
                )
                continue
            if parsed.fragment and target.suffix.lower() == ".html":
                if parsed.fragment not in known_ids.get(target, set()):
                    errors.append(
                        f"{page.relative_to(site)}: missing anchor {reference!r}"
                    )
    return errors


def build(source: Path, output: Path, downloads: Path, pdf: Path) -> None:
    if not source.is_dir():
        raise SystemExit(f"HTML website source directory does not exist: {source}")

    missing_pages = sorted(name for name in REQUIRED_PAGES if not (source / name).is_file())
    if missing_pages:
        raise SystemExit("Missing hand-authored website pages: " + ", ".join(missing_pages))

    if not downloads.is_dir():
        raise SystemExit(f"Validated downloads directory does not exist: {downloads}")
    if not pdf.is_file():
        raise SystemExit(f"Generated language manual PDF does not exist: {pdf}")

    output.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, output, dirs_exist_ok=True)

    download_output = output / "assets" / "downloads"
    download_output.mkdir(parents=True, exist_ok=True)
    for item in downloads.iterdir():
        if item.is_file():
            shutil.copy2(item, download_output / item.name)

    pdf_output = output / "pdf" / "NEXUS-Documentation.pdf"
    pdf_output.parent.mkdir(parents=True, exist_ok=True)
    if pdf.resolve() != pdf_output.resolve():
        shutil.copy2(pdf, pdf_output)

    names = asset_names(ROOT)
    for key in ("setup", "portable", "vsix", "paint_executable", "paint_archive"):
        artifact = names[key]
        for filename in (artifact, artifact + ".sha256.txt"):
            if not (download_output / filename).is_file():
                raise SystemExit(f"Required published download is missing: {filename}")

    (output / ".nojekyll").write_text("", encoding="utf-8")
    errors = audit_site(output)
    if errors:
        raise SystemExit("HTML link validation failed:\n  " + "\n  ".join(errors))

    print(f"PASS copied {len(list(source.rglob('*.html')))} hand-authored HTML pages")
    print(f"PASS copied {len(list(download_output.iterdir()))} download files")
    print("PASS manual PDF is present")
    print("PASS all local HTML, stylesheet, script, image and download links resolve")
    print("PASS site is plain HTML/CSS/JS; no theme or template renderer is used")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "website")
    parser.add_argument("--output", type=Path, default=ROOT / "site")
    parser.add_argument("--downloads", type=Path, default=ROOT / "docs/assets/downloads")
    parser.add_argument("--pdf", type=Path, default=ROOT / "site/pdf/NEXUS-Documentation.pdf")
    args = parser.parse_args()
    build(
        args.source.resolve(),
        args.output.resolve(),
        args.downloads.resolve(),
        args.pdf.resolve(),
    )


if __name__ == "__main__":
    main()
