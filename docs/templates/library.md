# Library Template

The `library` template currently seeds a single `src/main.nx` file with a reusable `add(a, b)` function. It creates the normal project manifest and directory structure, but does not generate a public module tree, versioned API surface, test modules or a published package registry entry.

## Create and check

```bash
nexus new MyLibrary --template library
cd MyLibrary
nexus check
nexus build
```

The current source is `examples/templates/library.nx`:

```nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}
```

Expand the source with the functions your package needs and add independent test programs under the repository's test conventions. The `nexus pkg` commands currently focus on local package metadata/vendoring and archive creation; they do not upload a package to a hosted registry. See [CLI commands](../tooling/commands.md#package-commands).
