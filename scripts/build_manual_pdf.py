#!/usr/bin/env python3
"""Render the NEXUS language manual to a readable PDF for the documentation site."""
from __future__ import annotations

import argparse
import html
import re
from pathlib import Path
from urllib.parse import urljoin, urlsplit, urlunsplit

import markdown
from pygments.formatters import HtmlFormatter
from weasyprint import HTML


SITE_BASE = "https://fanxcz.github.io/nexus-lang/"


def site_url_for_link(raw_url: str) -> str:
    """Make cross-document Markdown links absolute site links; keep page-local anchors."""
    if not raw_url or raw_url.startswith("#") or raw_url.startswith("data:"):
        return raw_url
    parsed = urlsplit(raw_url)
    if parsed.scheme or parsed.netloc:
        return raw_url

    resolved = urlsplit(urljoin(SITE_BASE, raw_url))
    path = resolved.path
    if path.lower().endswith(".md"):
        path_without_suffix = path[:-3]
        if path_without_suffix.endswith("/index"):
            path = path_without_suffix[:-5] + "/"
        else:
            path = path_without_suffix.rstrip("/") + "/"
        resolved = resolved._replace(path=path)
    return urlunsplit(resolved)


def rewrite_relative_hrefs(document: str) -> str:
    """Prevent cross-page Markdown links from becoming missing local PDF anchors."""
    pattern = re.compile(r'(\bhref=")([^"]*)(")')
    return pattern.sub(lambda match: match.group(1) + html.escape(site_url_for_link(html.unescape(match.group(2))), quote=True) + match.group(3), document)


def build_pdf(source: Path, output: Path) -> None:
    if not source.is_file():
        raise SystemExit(f"Manual source does not exist: {source}")

    markdown_text = source.read_text(encoding="utf-8")
    rendered = markdown.markdown(
        markdown_text,
        extensions=["extra", "toc", "codehilite", "sane_lists"],
        output_format="html5",
    )
    rendered = rewrite_relative_hrefs(rendered)

    style = r"""
    @page {
      size: A4;
      margin: 18mm 16mm 19mm;
      @top-right {
        content: "NEXUS · Language Manual";
        color: #63758a;
        font-size: 8pt;
      }
      @bottom-right {
        content: counter(page);
        color: #63758a;
        font-size: 8pt;
      }
    }
    @page:first {
      @top-right { content: ""; }
      @bottom-right { content: ""; }
    }
    html { font-size: 10pt; }
    body {
      margin: 0;
      color: #1a2b40;
      font-family: "DejaVu Sans", "Liberation Sans", sans-serif;
      line-height: 1.55;
      overflow-wrap: anywhere;
    }
    .cover {
      min-height: 230mm;
      display: flex;
      flex-direction: column;
      justify-content: center;
      page-break-after: always;
      border-bottom: 3px solid #087f94;
    }
    .cover .eyebrow {
      color: #087f94;
      font-size: 10pt;
      font-weight: 700;
      letter-spacing: .15em;
      text-transform: uppercase;
    }
    .cover h1 {
      max-width: 150mm;
      color: #17345b;
      font-size: 32pt;
      line-height: 1.08;
      margin: 12mm 0 5mm;
      border: 0;
    }
    .cover p { color: #53657a; font-size: 13pt; max-width: 145mm; }
    .cover .version { margin-top: 16mm; font-size: 9pt; color: #6b7a8d; }
    h1, h2, h3, h4 { color: #17345b; font-weight: 700; page-break-after: avoid; }
    h1 { font-size: 23pt; border-bottom: 1px solid #d9e2ec; padding-bottom: 3mm; }
    h2 { font-size: 17pt; margin-top: 10mm; padding-bottom: 2mm; border-bottom: 1px solid #d9e2ec; }
    h3 { font-size: 13pt; margin-top: 7mm; }
    h4 { font-size: 11pt; }
    p { margin: 3mm 0; }
    a { color: #145ba0; text-decoration: none; overflow-wrap: anywhere; }
    strong { color: #172b45; }
    code {
      font-family: "DejaVu Sans Mono", "Liberation Mono", monospace;
      font-size: 8.5pt;
      color: #17345b;
      background: #eef3f8;
      padding: .3mm 1mm;
    }
    pre {
      page-break-inside: avoid;
      white-space: pre-wrap;
      overflow-wrap: anywhere;
      border: 1px solid #d9e2ec;
      border-radius: 2mm;
      background: #f4f7fb;
      padding: 3mm;
      font-size: 8pt;
      line-height: 1.4;
    }
    pre code { background: transparent; padding: 0; color: #1a2b40; }
    table { width: 100%; border-collapse: collapse; margin: 4mm 0; font-size: 8.5pt; }
    th, td { border: 1px solid #d9e2ec; padding: 2mm; vertical-align: top; }
    th { background: #eaf3f8; color: #17345b; text-align: left; }
    blockquote { margin: 4mm 0; padding: 1mm 4mm; border-left: 3px solid #087f94; color: #53657a; }
    ul, ol { padding-left: 7mm; }
    li { margin: 1mm 0; }
    hr { border: 0; border-top: 1px solid #d9e2ec; margin: 8mm 0; }
    img { max-width: 100%; height: auto; }
    .toc { padding: 4mm; background: #f4f7fb; border: 1px solid #d9e2ec; border-radius: 2mm; }
    .toc ul { margin: 1mm 0; }
    .codehilite { page-break-inside: avoid; }
    """
    pygments_css = HtmlFormatter(style="default").get_style_defs(".codehilite")
    document = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>NEXUS Language Manual</title>
<style>{style}\n{pygments_css}</style>
</head>
<body>
<section class="cover">
  <div class="eyebrow">NEXUS Programming Language</div>
  <h1>Language Manual</h1>
  <p>Compiler usage, syntax, standard library, runtime, development workflow, and documented feature limits.</p>
  <div class="version">Generated from the current repository manual · fanxcz/nexus-lang</div>
</section>
<main>{rendered}</main>
</body>
</html>
"""
    output.parent.mkdir(parents=True, exist_ok=True)
    HTML(string=document, base_url=str(source.resolve().parent)).write_pdf(str(output))
    if not output.is_file() or output.stat().st_size < 4096:
        raise SystemExit(f"PDF generation failed or created an unexpectedly small file: {output}")
    print(f"PASS generated {output} ({output.stat().st_size} bytes)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Markdown manual")
    parser.add_argument("output", type=Path, help="Generated PDF path")
    args = parser.parse_args()
    build_pdf(args.source, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
