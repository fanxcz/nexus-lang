---
hide:
  - navigation
  - toc
---

<div class="nx-landing" markdown>

<div class="nx-hero" markdown>

<div class="nx-hero__copy" markdown>

<p class="nx-kicker"><span class="nx-kicker__dot"></span> NEXUS 1.1 · OPEN SOURCE · C++20</p>

# Code closer to the machine.

NEXUS is a compiled programming language and native runtime for building applications, creative tools, and games — with a compiler you can inspect and a workflow you can use today.

<div class="nx-hero__actions" markdown>
[Start building](getting-started/quickstart.md){ .md-button .md-button--primary }
[Explore on GitHub](https://github.com/fanxcz/nexus-lang){ .md-button }
</div>

<p class="nx-hero__meta">MIT licensed <span>·</span> Windows <span>·</span> Linux <span>·</span> macOS source builds</p>

</div>

<div class="nx-terminal" markdown>
<div class="nx-terminal__top"><span></span><span></span><span></span><b>hello.nx</b><small>EXAMPLE 001</small></div>

<pre><code class="language-nx">fn main() {
    let tool = "NEXUS"
    print("Hello from " + tool + "!")
}</code></pre>

<div class="nx-terminal__bottom"><span class="nx-terminal__check">✓</span> Source → compiler → native program</div>
</div>

</div>

<div class="nx-signal-row" markdown>
<div><span class="nx-signal__label">01 / LANGUAGE</span><strong>Readable syntax</strong><p>Functions, typed bindings, control flow, modules, structs, and enums.</p></div>
<div><span class="nx-signal__label">02 / COMPILER</span><strong>Two compilation paths</strong><p>A built-in x86-64 backend for its documented subset, plus an LLVM/Clang path.</p></div>
<div><span class="nx-signal__label">03 / RUNTIME</span><strong>Beyond the terminal</strong><p>Foundations for graphics, audio, input, UI, ECS, particles, and physics.</p></div>
</div>

<div class="nx-section-heading" markdown>
<p class="nx-kicker">A PRACTICAL TOOLCHAIN</p>

## Start with an idea. Build something real.

NEXUS brings the everyday parts of development into one CLI: project templates, compiler diagnostics, formatting, linting, tests, documentation, and editor integration.
</div>

<div class="nx-feature-grid" markdown>
<div class="nx-feature" markdown>
<div class="nx-feature__icon">⌘</div>
<p class="nx-feature__eyebrow">01 · BUILD</p>

### A direct workflow

Create a project, check it, build it, and run it without learning a sprawling toolchain first.

[Quick start →](getting-started/quickstart.md)
</div>
<div class="nx-feature" markdown>
<div class="nx-feature__icon">⌁</div>
<p class="nx-feature__eyebrow">02 · LANGUAGE</p>

### A language you can inspect

The compiler and runtime are written in C++20. The source, tests, and design notes are public.

[Language reference →](language/index.md)
</div>
<div class="nx-feature" markdown>
<div class="nx-feature__icon">▦</div>
<p class="nx-feature__eyebrow">03 · CREATE</p>

### Apps, tools, and games

Explore starter templates and runtime examples before assembling your own project.

[Browse templates →](templates/index.md)
</div>
<div class="nx-feature" markdown>
<div class="nx-feature__icon">↗</div>
<p class="nx-feature__eyebrow">04 · DEVELOP</p>

### Tools that fit together

Use \`fmt\`, \`lint\`, \`doc\`, \`bench\`, \`doctor\`, and LSP integration in a growing workflow.

[CLI command guide →](tooling/commands.md)
</div>
</div>

<div class="nx-start-grid" markdown>
<div class="nx-start-copy" markdown>
<p class="nx-kicker">YOUR FIRST FIVE MINUTES</p>

## From fresh folder to running code

Start from the CLI template. The same commands work from a clean project created by NEXUS.

For quick calculations, run `nexus` by itself or use `nexus repl`. The interactive shell
evaluates expressions with the NEXUS compiler and keeps saved `let` bindings for the session.

</div>
<div class="nx-start-code" markdown>
<div class="nx-code-label"><span>TERMINAL</span><span>POWERSHELL · BASH</span></div>

<pre><code class="language-bash">nexus new HelloNexus --template cli
cd HelloNexus
nexus build
nexus run</code></pre>

<p>Working with a single source file? Try <code>nexus run hello.nx --backend native</code> for programs supported by the built-in backend.</p>
</div>
</div>

<div class="nx-note" markdown>
<div class="nx-note__mark">i</div>
<div markdown>
**Know what is supported.** The built-in backend currently emits x86-64 Windows PE and Linux ELF executables for a defined language subset. Other programs may need the LLVM/Clang toolchain. Runtime features and targets have individual limitations — the docs call them out instead of pretending every path is finished.

[Understand backend selection →](tooling/backend-explainer.md)
</div>
</div>

<div class="nx-bottom-cta" markdown>
<div markdown>
<p class="nx-kicker">READY WHEN YOU ARE</p>

## Get the tools. Read the source. Make it yours.

Choose the Windows installer or portable package, browse the releases for other systems, or contribute directly to the compiler.
</div>
<div class="nx-bottom-cta__actions" markdown>
[Downloads & installers](downloads.md){ .md-button .md-button--primary }
[Read the roadmap](development/roadmap.md){ .md-button }
</div>
</div>

</div>
