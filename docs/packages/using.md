# Using Packages

NEXUS currently supports **local vendored packages**. Git dependencies, version-range resolution and a hosted registry are not implemented. The local package commands update `nexus.toml`, copy source into `vendor/`, and refresh `nexus.lock`.

## Add a local package

The source directory must contain `nexus.toml` and use a package name made from letters, digits, underscores or hyphens.

```sh
nexus pkg add ../my-library
nexus pkg list
```

The source is copied to `vendor/<package-name>/`. Transient `.git`, `.vscode`, `build`, `dist`, `node_modules` and `CMakeFiles` directories and symlinks are not copied. Adding the current project as its own dependency, or choosing a source that contains the destination vendor path, is rejected.

## Remove a package

```sh
nexus pkg remove my-library
```

The command removes `vendor/my-library`, deletes that package's entry from the `[dependencies]` section in `nexus.toml`, and regenerates `nexus.lock`. Path separators and traversal names are rejected; unrelated dependencies are preserved.

## Lockfile behavior

The lockfile records the project name/version and dependency versions. For vendored local packages, NEXUS reads the version from the package's own manifest:

```toml
# NEXUS lockfile v1
package = "my-app"
version = "1.0.0"

[dependencies]
my-library = "0.3.0"
```

Regenerate the lockfile from `nexus.toml` with:

```sh
nexus pkg update
```

## Current limitations

This is not a full package solver yet. There are no remote registries, Git fetching, semver range resolution, transitive dependency solver, checksums, or guaranteed reproducible source verification. `nexus pkg publish` creates a local ZIP-formatted `.nxpkg` archive under `dist/`; it does not upload anything to a server. `nexus pkg build` builds the configured entry source, not a dependency graph.

See [Creating Packages](creating.md) and the [development roadmap](../development/roadmap.md).
