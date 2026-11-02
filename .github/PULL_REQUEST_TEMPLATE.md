## Description

Please include a summary of the change and which issue is fixed. Include relevant motivation and context.

Fixes # (issue number)

## Type of Change

- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Code refactoring (no functional changes)
- [ ] Test addition / improvement
- [ ] CI / Build system change
- [ ] Dependency update

## Component(s) Affected

- [ ] Compiler (lexer, parser, semantic, codegen)
- [ ] Runtime (memory, graphics, audio, input, etc.)
- [ ] Standard Library (std.io, std.fs, std.net, etc.)
- [ ] Tooling (fmt, lint, doc, bench, lsp, editor)
- [ ] Package Manager
- [ ] Build System / CMake / Templates
- [ ] Examples / Tests
- [ ] Documentation
- [ ] CI / Infrastructure

## Testing

Please describe the tests you ran to verify your changes. Provide instructions so we can reproduce.

- [ ] Unit tests pass (`ctest --test-dir build --output-on-failure`)
- [ ] Smoke tests pass (`./build/nexus --version`)
- [ ] Project smoke test passes (create/build/run a test project)
- [ ] Paint smoke test passes
- [ ] Manual testing performed (describe below)

### Test Configuration
- OS: [e.g., Ubuntu 24.04, Windows 11, macOS 14]
- Compiler: [e.g., clang 18, MSVC 19.40, GCC 13]
- Build Type: [Debug / Release / RelWithDebInfo]

## Checklist

- [ ] My code follows the style guidelines of this project (`nexus fmt`)
- [ ] I have performed a self-review of my code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing unit tests pass locally with my changes
- [ ] Any dependent changes have been merged and published

## Screenshots / Recordings (if applicable)

Add screenshots or GIFs demonstrating the change, especially for:
- Graphics / UI changes
- Editor / LSP features
- New tooling output
- Runtime behavior

## Additional Notes

Any additional information, configuration, or data that might be necessary to reproduce or understand the change.

## Engineering checks

- [ ] I read the relevant architecture and backend-support documentation.
- [ ] NEXUS examples and `stdlib/` changes follow [style-guide-nx.md](../docs/style-guide-nx.md).
- [ ] I added/updated focused unit, negative, fuzz-seed or differential tests as appropriate.
- [ ] I ran relevant CTest checks and `git diff --check`.
- [ ] C++ changes pass `clang-format --dry-run --Werror` and applicable `clang-tidy` checks.
- [ ] Backend and platform support claims are backed by tests; uncertain claims are marked `Needs verification`.
- [ ] I did not include credentials, generated binaries, private logs or unrelated files.

For language changes, link the related RFC/Discussion and explain compatibility impact. For security-sensitive changes, use the private reporting process instead of public issue details.

