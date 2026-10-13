# Publishing Packages

## Current behavior

NEXUS does **not** have a hosted package registry. The implemented `nexus pkg publish` command creates a ZIP-formatted `.nxpkg` archive in the project's `dist/` directory; it does not upload it anywhere.

## Create a local archive

Run this from a directory containing a valid `nexus.toml` manifest:

```sh
nexus check
nexus fmt --check
nexus lint
nexus pkg publish
```

The formatter and linter reflect their current limited implementations; review their output and run the project's actual tests before distributing an archive. The archive is a local file that you can share separately.

## Add a package from source

A consumer project can vendor a local **source directory** with:

```sh
nexus pkg add ../my-library
nexus pkg list
```

The `pkg add` command expects a package directory, not the generated `.nxpkg` archive. See [Using Packages](using.md) for the supported local workflow.

## Not available yet

The CLI does not implement registry login, registry upload, remote package lookup, publish dry-runs, unpublishing/yanking, package ownership, private registries, or version-range solving. Commands such as `nexus pkg login`, `nexus pkg info`, and `nexus pkg unpublish` should not be used; they are not implemented.
