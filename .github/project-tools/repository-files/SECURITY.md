# Security Policy

## Reporting a vulnerability

Please do not publish an exploitable vulnerability, proof-of-concept with real targets, credentials, private data, or unreleased exploit details in a public issue.

Use GitHub's **Report a vulnerability** option in the repository's Security tab when it is available. If private reporting is unavailable, contact the repository owner through the [GitHub profile](https://github.com/fanxcz) and ask for a private reporting channel. Include the affected commit/version, platform, reproduction steps, impact, and any proposed mitigation. Avoid attaching secrets or personal data.

## Scope and expectations

Security-sensitive areas include source parsing, compiler code generation, generated native executables, package archive handling, filesystem operations, process execution, runtime FFI, networking, and installer/update scripts.

NEXUS is under active development; not every component has a formal security audit. Do not assume sandboxing or isolation unless a feature's documentation and tests explicitly establish it.

## Safe development

- Never commit access tokens, signing keys, credentials or private build logs.
- Treat project manifests, package contents, FFI declarations and native code as untrusted input.
- Avoid automatically executing package-provided build steps.
- Validate paths and archive entries before filesystem writes.
- Document security limitations and add regression tests when fixing a vulnerability.
