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

## Threat model

The project treats source files, project manifests, imports, package archives and compiler inputs as untrusted. In-scope reports include:

- Malicious `.nx` source that triggers a compiler memory-safety bug, parser/lexer crash, uncontrolled resource exhaustion or unsafe compiler behavior.
- Incorrect code generation that causes a generated executable to violate the language/runtime contract in a way that can be exploited, such as memory corruption or unintended process/file access beyond what the source program requested.
- Path traversal, unsafe archive extraction, command execution or defects in package/project tooling, runtime FFI boundaries, installers or update scripts.
- A compiler diagnostic or backend discrepancy that allows malformed input to bypass a safety check and produce unsafe output.

The compiler is not a sandbox. Programs that intentionally request unsafe operations through documented APIs, third-party libraries, operating-system behavior or vulnerabilities unrelated to NEXUS are out of scope unless NEXUS introduces or materially amplifies the defect. Do not assume generated executables are isolated from their host or from user permissions.

## Generated binary disclosure and classification

When a generated executable exhibits unexpected or unsafe behavior, first classify whether the behavior is explicitly requested by the input program or comes from incorrect compiler/runtime behavior. A suspected compiler defect is in scope even if the triggering source is intentionally malformed or adversarial.

Report suspected unsafe generated behavior privately. Include, where safe: repository commit or release version, OS and target triple, selected backend (`native`, `llvm` or `auto`), compiler/toolchain versions, a minimal source reproducer, exact build/run commands, observed versus expected behavior, and reproducibility. Attach only harmless test binaries or minimized inputs; do not attach credentials, private data, live malware or payloads targeting third-party systems. If a binary itself is unsafe, do not execute it on a production machine; use an isolated disposable environment and describe the isolation used.

Do not publish an exploitable proof of concept, malicious generated binary, crash artifact with exploitability or detailed trigger in a public issue or Discussion. The existing contact routes above remain authoritative; this policy does not promise a fixed response SLA.

## Coordinated disclosure

After a fix is available, the maintainer may publish an advisory, affected versions, mitigation guidance and a regression test. Security fixes should not disclose weaponizable details before users have a reasonable opportunity to update. Add a minimized regression input to the public test suite only after confirming it no longer exposes an exploitable behavior.

