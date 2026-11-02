---
hide:
  - navigation
  - toc
  - footer
---

<div class="nexus-home">
  <header class="nexus-site-header">
    <a class="nexus-brand" href="https://fanxcz.github.io/nexus-lang/./" aria-label="NEXUS home">
      <img src="assets/images/nexus-n-2026.svg" alt="" width="34" height="34">
      <span>NEXUS</span>
      <span class="nexus-brand-divider"></span>
      <small>LANGUAGE & TOOLCHAIN</small>
    </a>
    <nav class="nexus-site-nav" aria-label="Main navigation">
      <a href="https://fanxcz.github.io/nexus-lang/manual/">Language</a>
      <a href="https://fanxcz.github.io/nexus-lang/applications/">Applications</a>
      <a href="https://fanxcz.github.io/nexus-lang/examples/overview/">Examples</a>
      <a href="https://fanxcz.github.io/nexus-lang/studio/">Studio</a>
      <a href="https://fanxcz.github.io/nexus-lang/downloads/">Downloads</a>
    </nav>
    <a class="nexus-nav-cta" href="https://fanxcz.github.io/nexus-lang/getting-started/quickstart/">Get started <span>↗</span></a>
  </header>

  <main>
    <section class="nexus-hero">
      <div class="nexus-hero-copy">
        <div class="nexus-overline"><span class="nexus-live-dot"></span> OPEN SOURCE · VERSION 2.2.0</div>
        <h1>Native software.<br><em>Clear by design.</em></h1>
        <p class="nexus-hero-intro">NEXUS is a statically checked language and C++20 toolchain for desktop software, developer tools and small games. Start with a plain source file. Keep control of what gets built.</p>
        <div class="nexus-hero-actions">
          <a class="nexus-button nexus-button-primary" href="https://fanxcz.github.io/nexus-lang/getting-started/quickstart/">Write your first program <span>↗</span></a>
          <a class="nexus-button nexus-button-quiet" href="https://fanxcz.github.io/nexus-lang/downloads/">Download compiler</a>
        </div>
        <div class="nexus-hero-footnote">Compiler · Runtime · Standard library · Desktop tools</div>
      </div>

      <div class="nexus-code-stage" aria-label="Illustrative NEXUS source editor preview">
        <div class="nexus-code-window">
          <div class="nexus-window-bar">
            <div class="nexus-window-dots"><i></i><i></i><i></i></div>
            <span class="nexus-window-file">examples / hello.nx</span>
            <span class="nexus-window-mode">NEXUS</span>
          </div>
          <div class="nexus-code-body">
            <div class="nexus-code-line"><span class="nexus-line-number">01</span><code><b>fn</b> main() {</code></div>
            <div class="nexus-code-line"><span class="nexus-line-number">02</span><code>&nbsp;&nbsp;<b>let</b> language = <i>"NEXUS"</i></code></div>
            <div class="nexus-code-line"><span class="nexus-line-number">03</span><code>&nbsp;&nbsp;print(<i>"Hello from "</i> + language)</code></div>
            <div class="nexus-code-line"><span class="nexus-line-number">04</span><code>}</code></div>
            <div class="nexus-code-cursor"></div>
          </div>
          <div class="nexus-code-output">
            <div class="nexus-output-heading"><span>LOCAL WORKFLOW</span><span>READY</span></div>
            <div class="nexus-output-step"><b>01</b><span>Check source</span><span class="nexus-step-state">nexus check</span></div>
            <div class="nexus-output-step"><b>02</b><span>Build executable</span><span class="nexus-step-state">nexus build</span></div>
            <div class="nexus-output-step"><b>03</b><span>Run the program</span><span class="nexus-step-state">nexus run</span></div>
          </div>
        </div>
        <div class="nexus-code-caption"><span>01 / A small program</span><span>Source preview, not a product screenshot</span></div>
      </div>
    </section>

    <section class="nexus-facts" aria-label="NEXUS platform facts">
      <div><strong>C++20</strong><span>Compiler implementation</span></div>
      <div><strong>2 paths</strong><span>Native emitter and LLVM</span></div>
      <div><strong>x86-64</strong><span>Built-in Windows and Linux targets</span></div>
      <div><strong>SDL-based</strong><span>2D runtime and desktop UI helpers</span></div>
    </section>

    <section class="nexus-section nexus-approach">
      <div class="nexus-section-heading">
        <div class="nexus-overline">01 / THE TOOLCHAIN</div>
        <h2>One language.<br><em>Two ways to compile.</em></h2>
      </div>
      <div class="nexus-approach-content">
        <p class="nexus-section-lead">The frontend checks the program once. Choose the backend that fits the work, or let the driver select a supported path.</p>
        <div class="nexus-backend-grid">
          <article class="nexus-backend-card">
            <div class="nexus-card-topline"><span>BACKEND A</span><span class="nexus-card-mark">N</span></div>
            <h3>Built-in native</h3>
            <p>Direct x86-64 code generation for a deliberately documented language subset. No separately installed LLVM compiler is needed for supported programs.</p>
            <div class="nexus-card-meta"><span>PE32+ / Windows</span><span>ELF64 / Linux</span></div>
            <a href="https://fanxcz.github.io/nexus-lang/tooling/native-backend/">Native backend notes <span>↗</span></a>
          </article>
          <article class="nexus-backend-card nexus-backend-card-light">
            <div class="nexus-card-topline"><span>BACKEND B</span><span class="nexus-card-mark">IR</span></div>
            <h3>LLVM compilation</h3>
            <p>LLVM IR is the broader code-generation path for features outside the native subset. A compatible Clang-style C++ toolchain is required.</p>
            <div class="nexus-card-meta"><span>Feature coverage varies</span><span>Toolchain required</span></div>
            <a href="https://fanxcz.github.io/nexus-lang/backend-support-matrix/">Read the support matrix <span>↗</span></a>
          </article>
        </div>
        <p class="nexus-caveat">Backend support is not identical. The parser accepting a construct does not mean both backends can emit it; check the current <a href="https://fanxcz.github.io/nexus-lang/backend-support-matrix/">support matrix</a> before choosing a target.</p>
      </div>
    </section>

    <section class="nexus-section nexus-work">
      <div class="nexus-section-heading">
        <div class="nexus-overline">02 / WHAT YOU CAN BUILD</div>
        <h2>From a small tool<br><em>to a real desktop app.</em></h2>
      </div>
      <div class="nexus-work-grid">
        <article class="nexus-work-card nexus-work-studio">
          <div class="nexus-work-index">01 — DESKTOP WORKSPACE</div>
          <div class="nexus-work-art nexus-studio-art" aria-hidden="true">
            <div class="nexus-art-sidebar"><span></span><span></span><span></span><span></span></div>
            <div class="nexus-art-editor">
              <div class="nexus-art-tab">main.nx</div>
              <div class="nexus-art-code"><i></i><i></i><i></i><i></i><i></i></div>
              <div class="nexus-art-terminal"><span>BUILD OUTPUT</span><b>Build completed</b></div>
            </div>
          </div>
          <h3>NEXUS Studio</h3>
          <p>A desktop workspace with a project explorer, NEXUS-aware editor, integrated terminal and direct Build/Run actions. The compiler is installed separately.</p>
          <a href="https://fanxcz.github.io/nexus-lang/studio/">Explore Studio <span>↗</span></a>
        </article>
        <article class="nexus-work-card">
          <div class="nexus-work-index">02 — CREATIVE TOOL</div>
          <div class="nexus-work-art nexus-paint-art" aria-hidden="true">
            <div class="nexus-paint-toolbar"><i></i><i></i><i></i><i></i><i></i></div>
            <div class="nexus-paint-canvas"><span></span><b></b><i></i></div>
            <div class="nexus-paint-palette"><i></i><i></i><i></i><i></i><i></i></div>
          </div>
          <h3>NEXUS Paint</h3>
          <p>A native raster-painting example with brush, eraser, geometric tools, fill, undo/redo and BMP save/load.</p>
          <a href="https://fanxcz.github.io/nexus-lang/downloads/">Windows downloads <span>↗</span></a>
        </article>
        <article class="nexus-work-card">
          <div class="nexus-work-index">03 — GAMES & GRAPHICS</div>
          <div class="nexus-work-art nexus-game-art" aria-hidden="true">
            <div class="nexus-game-grid"></div><span class="nexus-game-player"></span><i class="nexus-game-enemy"></i><b class="nexus-game-orbit"></b>
          </div>
          <h3>Games and experiments</h3>
          <p>Playable prototypes cover vector movement, tilemaps, collision, cameras, particles, audio and an experimental 3D path.</p>
          <a href="https://fanxcz.github.io/nexus-lang/examples/overview/">Browse source examples <span>↗</span></a>
        </article>
      </div>
    </section>

    <section class="nexus-section nexus-developer">
      <div class="nexus-section-heading">
        <div class="nexus-overline">03 / THE DAILY WORK</div>
        <h2>A small, explicit<br><em>developer workflow.</em></h2>
      </div>
      <div class="nexus-developer-content">
        <div class="nexus-terminal-card">
          <div class="nexus-terminal-title"><span>TERMINAL</span><span>PROJECT ROOT</span></div>
          <p><i>$</i> nexus check src/main.nx</p>
          <p class="nexus-terminal-muted">Checking source and types…</p>
          <p><i>$</i> nexus build src/main.nx --backend auto</p>
          <p class="nexus-terminal-success">Build finished · executable written</p>
          <p><i>$</i> nexus run src/main.nx</p>
          <p class="nexus-terminal-muted">Program output appears here.</p>
        </div>
        <div class="nexus-developer-notes">
          <article><b>01</b><div><h3>Language tooling</h3><p>Diagnostics, completion, hover, formatting, workspace symbols and quick fixes through the NEXUS language server.</p><a href="https://fanxcz.github.io/nexus-lang/tooling/lsp/">Language server reference ↗</a></div></article>
          <article><b>02</b><div><h3>Editor integration</h3><p>VS Code support for <code>.nx</code> files, syntax highlighting, snippets and LSP features.</p><a href="https://fanxcz.github.io/nexus-lang/tooling/vscode-extension/">VS Code setup ↗</a></div></article>
          <article><b>03</b><div><h3>Libraries and runtime</h3><p>Strings, files, JSON, networking, SQLite, memory helpers, 2D graphics and immediate-mode UI.</p><a href="https://fanxcz.github.io/nexus-lang/std/">Browse the standard library ↗</a></div></article>
        </div>
      </div>
    </section>

    <section class="nexus-bottom-cta">
      <div><div class="nexus-overline">START WITH THE SOURCE</div><h2>Make the first build<br><em>your own.</em></h2></div>
      <div class="nexus-bottom-copy"><p>Install the compiler, open a small example and learn the language by changing something that runs.</p><div class="nexus-hero-actions"><a class="nexus-button nexus-button-primary" href="https://fanxcz.github.io/nexus-lang/downloads/">Get NEXUS <span>↗</span></a><a class="nexus-button nexus-button-dark" href="https://fanxcz.github.io/nexus-lang/manual/">Read the manual</a></div></div>
    </section>
  </main>

  <footer class="nexus-site-footer">
    <a class="nexus-brand" href="https://fanxcz.github.io/nexus-lang/./"><img src="assets/images/nexus-n-2026.svg" alt="" width="28" height="28"><span>NEXUS</span></a>
    <span>Open-source language and toolchain · MIT License</span>
    <div><a href="https://github.com/fanxcz/nexus-lang">Source repository ↗</a><a href="https://github.com/fanxcz/nexus-lang/blob/main/SECURITY.md">Security</a></div>
  </footer>
</div>
