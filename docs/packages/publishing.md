# Publishing Packages

Sharing packages with the community (planned feature).

## Current Status

Package registry is **not yet implemented** in v1.1.0. Currently only local packages supported.

## Planned Workflow

```bash
# 1. Prepare package
cd my-package
nexus test
nexus fmt --check
nexus lint
nexus doc

# 2. Login to registry
nexus pkg login
# Or with token
nexus pkg login --token $REGISTRY_TOKEN

# 3. Publish
nexus pkg publish
# Or specific version
nexus pkg publish --version 1.0.0

# 4. Verify
nexus pkg info my-package
```

## Package Requirements

Before publishing:

- [ ] Valid `nexus.toml` with all required fields
- [ ] All tests pass (`nexus test`)
- [ ] Code formatted (`nexus fmt --check`)
- [ ] No lint warnings (`nexus lint`)
- [ ] Documentation generated (`nexus doc`)
- [ ] README.md with usage examples
- [ ] LICENSE file
- [ ] CHANGELOG.md
- [ ] Version follows SemVer

## Publishing Checklist

```bash
# Automated check
nexus pkg publish --dry-run
```

Checks:
- Package name available
- Version not already published
- All files included
- No sensitive files (`.env`, secrets, etc.)
- Size limits respected

## Registry Configuration

```toml
# nexus.toml
[registries]
default = "https://packages.nexus-lang.org"
private = "https://pkg.company.com"
```

## Authentication

```bash
# Token-based
nexus pkg login --token $TOKEN

# Or config file
# ~/.config/nexus/auth.toml
[registries.default]
token = "..."

[registries.private]
token = "..."
```

## Unpublishing

```bash
# Within 24 hours (or with admin approval)
nexus pkg unpublish my-package@1.0.0

# Yank (mark as deprecated, don't delete)
nexus pkg yank my-package@1.0.0 --reason "Critical bug"
```

## Ownership

```bash
# Add owner
nexus pkg owner add my-package @user

# Remove owner
nexus pkg owner remove my-package @user

# List owners
nexus pkg owner list my-package
```

## Private Packages

```toml
# nexus.toml
[package]
publish = false  # Never publish to public registry

# Or specific registries
publish = ["private"]
```

## CI/CD Publishing

```yaml
# .github/workflows/publish.yml
on:
  release:
    types: [published]

jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: nexus-lang/setup-nexus@v1
      - run: nexus pkg login --token ${{ secrets.REGISTRY_TOKEN }}
      - run: nexus pkg publish
```

## Best Practices

1. **Automate with CI** - publish on GitHub release
2. **Sign releases** - GPG sign tags
3. **Maintain CHANGELOG** - clear migration guides
4. **Deprecate gracefully** - use `yank` for bad releases
5. **Respond to issues** - maintainers expected to triage