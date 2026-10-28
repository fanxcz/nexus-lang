# Package Manager Overview

NEXUS currently supports **local source-package vendoring**. It does not yet provide remote registry access or a full dependency solver, so the commands below describe the implemented local workflow only.

## What works today

- `nexus pkg add <directory>` copies a local package into the consuming project's `vendor/` directory and updates its manifest and lockfile.
- `nexus pkg list` lists local vendored packages.
- `nexus pkg remove <name>` removes the vendored package and its matching dependency entries.
- `nexus pkg update` regenerates the lockfile and SHA-256 checksums for local dependencies; it does not resolve remote or version-range dependencies.
- `nexus pkg verify` strictly parses dependency records and the generated v2 lockfile, rejects malformed entries and duplicate tables/keys, checks that locked project name/version and each vendored package name match the manifest, confines package paths to `vendor/`, and hashes local package contents.
- Package operations refuse a symlinked `vendor/` root. Replacing an existing package stages a backup and restores package/manifest/lockfile state if copying or lockfile refresh fails.
- `nexus pkg build` builds the package's configured entry source. It does not build a dependency graph.
- `nexus pkg publish` creates a local ZIP-formatted `.nxpkg` archive in `dist/`; it does not upload the archive.

## Create a local package

```sh
nexus new my-library --template library
cd my-library
nexus check
nexus build
nexus pkg publish
```

The project manifest is `nexus.toml`; the template starts with a source file under `src/`. Review the generated entry path before changing the package layout.

## Add a local package to another project

From an application project directory, pass the path to the package source directory:

```sh
nexus pkg add ../my-library
nexus pkg list
nexus pkg verify
```

The source tree is copied under `vendor/<package-name>/`. To remove it later, use `nexus pkg remove <package-name>`.

## Current limitations

The following capabilities are **not implemented**: a hosted registry, `nexus pkg login`, remote/Git dependencies, SemVer range solving, transitive dependency builds, signed packages, publisher authentication, reproducible archive verification, and registry operations such as unpublishing. Older examples that use those features will not work in the current NEXUS 2.2.0 project.

See [Creating Packages](creating.md), [Using Packages](using.md), and [Publishing Packages](publishing.md) for the current workflow.
