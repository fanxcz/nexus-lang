# Governance

## Project stewardship

NEXUS is maintained as an open-source project under the MIT License. The initial maintainer and final decision-maker for the project is **fanxcz** (`@fanxcz`). Maintainers may be added publicly as the project grows; until that happens, contributors should not assume that an automated review or a community vote constitutes approval.

The maintainer is responsible for the language and compiler roadmap, release decisions, security coordination, and resolving questions that cannot be settled by implementation evidence or existing policy. Contributors are encouraged to challenge designs constructively and provide benchmarks, tests, and compatibility analysis.

## Where to discuss work

- **Issues** are for reproducible bugs, build failures, regression reports, and concrete work items with acceptance criteria.
- **Discussions** are for questions, usage help, ideas, trade-offs, language design and early RFC discussion.
- **Private security reporting** is required for vulnerabilities and exploitable compiler defects. Follow [SECURITY.md](SECURITY.md).

Discussions must be enabled in repository settings; see [GitHub repository settings](docs/github-settings.md). If Discussions are not enabled yet, open a non-sensitive issue asking the maintainer to enable them rather than posting a security report or vague feature request.

## Language and compiler RFCs

A syntax, type-system, semantic, standard-library ABI, or backend-contract change that may affect existing programs should be proposed before implementation.

An RFC should include:

1. The problem and motivating examples, including current behavior.
2. Proposed syntax and semantics, with valid and invalid examples.
3. Compatibility impact and migration strategy.
4. Backend behavior (builtin native and LLVM) and target limitations.
5. Runtime, standard-library and tooling implications.
6. Diagnostics, tests, documentation and benchmark requirements.
7. Alternatives considered and unresolved questions.

Start in Discussions under **Ideas**. For substantial proposals, link a short RFC document or a draft pull request. The maintainer decides whether to accept, reject, defer, or request revision. Acceptance should be recorded in the discussion or linked issue before a breaking implementation is merged.

## Decision and review process

Focused bug fixes and documentation changes can be proposed directly as pull requests. Changes affecting public language behavior, backend output, package formats, or runtime APIs require explicit maintainer review. CI success is necessary but is not a substitute for design review.

A decision should consider correctness, security, maintainability, cross-platform behavior, backend parity, and compatibility with existing `.nx` programs. Where the backends intentionally differ, documentation and tests must make that difference explicit. The [backend support matrix](docs/backend-support-matrix.md) is the project-facing record of those limitations.

The maintainer may request changes, close out-of-scope proposals, or revert a regression. Contributors should use the [Code of Conduct](CODE_OF_CONDUCT.md) and [Contributing guide](CONTRIBUTING.md).

## Releases

The maintainer creates version tags and publishes release artifacts after reviewing the changelog, compatibility impact, CI results and SHA-256 checksums. The project follows [Semantic Versioning](docs/versioning.md) and [Keep a Changelog](CHANGELOG.md).

## Amendments

This document may be updated by a maintainer-reviewed pull request. Governance changes should be announced in Discussions when enabled.
