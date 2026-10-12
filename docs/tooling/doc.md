# Documentation Generator

`nexus doc` creates a basic Markdown API listing from the current project's configured entry source.

## Usage

Run this command in a directory containing `nexus.toml`:

```sh
nexus doc
```

NEXUS reads the file configured by `[build].entry` (default `src/main.nx`) and writes `docs/API.md`.

## What it extracts

- Consecutive lines starting with `///` are collected as documentation comments.
- Lines starting with `fn `, `struct `, or `enum ` become Markdown headings.
- The collected comment text is placed below the next recognized declaration.

For example:

```nx
/// Adds two integer values.
fn add(a: i64, b: i64) -> i64 {
    return a + b
}
```

The generated Markdown will include the declaration and the comment.

## Limitations

This is a small first-pass generator, not a full API documentation system. It does not currently produce HTML, parse nested module hierarchies, understand documentation attributes, validate links or examples, support custom output paths, watch files, or open a browser. Only declarations in the configured entry file are scanned.

See [Tooling Commands](commands.md) for the implemented CLI options and the [roadmap](../development/roadmap.md) for planned documentation improvements.
