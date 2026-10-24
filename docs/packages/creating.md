# Creating Packages

NEXUS's current package workflow is local-first: a package is a source directory with a `nexus.toml` file which another project can vendor with `nexus pkg add`.

## Minimum structure

```text
my-library/
├── nexus.toml
├── src/
│   └── main.nx
├── README.md
└── LICENSE
```

## Manifest

Start with a package created from a NEXUS template, or add a manifest like:

```toml
[package]
name = "my-library"
version = "1.0.0"

[build]
entry = "src/main.nx"
artifact = "my-library"
target = "native"
```

Package names must contain only letters, digits, underscores or hyphens. Ensure the configured entry file exists and passes `nexus check`.

## Local development workflow

```sh
# Create a package skeleton
nexus new my-library --template library
cd my-library
nexus check
nexus build

# In a separate application project:
nexus pkg add ../my-library
nexus pkg list
nexus pkg remove my-library
```

`nexus pkg add` copies files into the consuming project's `vendor/`, skipping Git/editor state, build output, distribution output and node modules. It writes a dependency entry into `nexus.toml` and refreshes the lockfile. Removing a package deletes its vendored source and clears the matching manifest/lockfile entries.

## Create a local archive

```sh
nexus pkg publish
```

This writes a ZIP-formatted `.nxpkg` archive into `dist/`. It does **not** publish to an online registry; registry authentication, uploads, version constraints, dependency solving and transitive builds are not implemented.

See [Using Packages](using.md) and the [NEXUS roadmap](../development/roadmap.md) for future package-manager goals.
