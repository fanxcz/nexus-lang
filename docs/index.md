---
hide:
  - navigation
  - toc
---

<div class="nx-hero" markdown>

<div class="nx-hero__copy" markdown>

<p class="nx-eyebrow">NATIVE LANGUAGE · APPS · GAMES</p>

# Meet NEXUS.

A programming language for turning ideas into native applications, creative tools, and games.

Write clean `.nx` code. Start with a lightweight built-in compiler backend, then use the LLVM pipeline when your program needs broader language and target coverage.

[Get started](getting-started/quickstart.md){ .md-button .md-button--primary }
[Downloads](downloads.md){ .md-button }
[View source](https://github.com/fanxcz/nexus-lang){ .md-button }

<p class="nx-platforms">Open source · MIT licensed · Windows · Linux · macOS source builds</p>

</div>

<div class="nx-hero__example" markdown>
<div class="nx-code-window">
  <div class="nx-code-window__bar"><span></span><span></span><span></span><b>hello.nx</b></div>
</div>
<pre><code class="language-nx">fn main() {
    let language = "NEXUS"
    print("Hello from " + language + "!")
}</code></pre>
<div class="nx-code-note">One small program. A native executable.</div>
</div>
</div>

<div class="nx-highlight-row" markdown>

<div markdown>
<span class="nx-number">01</span>

**Start small**

A readable language, direct command-line workflow, project templates, and fast feedback from the compiler.
</div>

<div markdown>
<span class="nx-number">02</span>

**Build real things**

A native runtime with 2D/3D graphics, audio, input, UI, ECS, particles, and physics foundations.
</div>

<div markdown>
<span class="nx-number">03</span>

**Grow with your project**

Use the built-in x86-64 backend for its supported subset, or LLVM for broader language and target coverage.
</div>

</div>

## From zero to a running program

Create a project from a template and build it:

<pre><code class="language-bash">nexus new HelloNexus --template cli
cd HelloNexus
nexus build
nexus run</code></pre>

You can also compile a single supported source file directly, with no separately installed compiler toolchain:

<pre><code class="language-bash">nexus run hello.nx --backend native</code></pre>

The built-in machine-code backend currently targets x86-64 Linux and Windows and supports a defined subset of NEXUS. Run <code>nexus explain-backend hello.nx</code> to see which backend will be selected and why.

## Made for makers

<div class="grid cards" markdown>

-   **Language and compiler**

    Learn functions, types, control flow, modules, diagnostics, and native code generation.

    [Read the language reference](language/overview.md)

-   **Games and graphics**

    Start from the game, 3D, or Paint template. Explore the graphics, input, audio, ECS, and physics APIs.

    [Explore the runtime](runtime/overview.md)

-   **Tools included**

    Format and lint code, generate docs, manage local packages, run benchmarks, and connect editor tooling through LSP foundations.

    [Explore the CLI](tooling/commands.md)

-   **Build with confidence**

    Cross-platform CI and regression tests validate the compiler. Backend parity tests compare output where both x86-64 backends are available.

    [See backend details](tooling/backend-explainer.md)

</div>

## Get NEXUS

<div class="nx-download-strip" markdown>
<div markdown>
### Windows

Installer and portable bundle for Windows x64. The portable package includes a bundled C++ toolchain and the runtime DLLs used by the Windows workflow.

[Windows downloads](downloads.md){ .md-button .md-button--primary }
</div>
<div markdown>
### Linux and macOS

The release workflow is configured to publish multi-platform archives containing a bundled Zig C++ compiler/linker for the LLVM compilation path. Those assets appear on GitHub Releases after a version tag is published. The built-in direct machine-code backend currently targets its documented x86-64 Linux/Windows subset; optional graphics/audio integrations may still depend on platform libraries.

[Platform releases](https://github.com/fanxcz/nexus-lang/releases){ .md-button }
</div>
</div>

## An open project

NEXUS is actively developed. Some advanced language features and runtime components are experimental; consult the relevant documentation before using them in production. Issues and contributions are welcome.

[Contribute on GitHub](https://github.com/fanxcz/nexus-lang){ .md-button }
[Report a problem](https://github.com/fanxcz/nexus-lang/issues){ .md-button }
