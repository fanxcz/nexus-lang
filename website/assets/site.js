(() => {
  const toggle = document.querySelector('[data-menu-toggle]');
  const nav = document.querySelector('[data-site-nav]');

  if (toggle && nav) {
    toggle.addEventListener('click', () => {
      const open = toggle.getAttribute('aria-expanded') === 'true';
      toggle.setAttribute('aria-expanded', String(!open));
      nav.classList.toggle('is-open', !open);
    });
    nav.querySelectorAll('a').forEach((link) => {
      link.addEventListener('click', () => {
        toggle.setAttribute('aria-expanded', 'false');
        nav.classList.remove('is-open');
      });
    });
  }

  document.querySelectorAll('[data-copy]').forEach((button) => {
    button.addEventListener('click', async () => {
      const target = document.getElementById(button.dataset.copy);
      if (!target) return;
      const original = button.textContent;
      try {
        await navigator.clipboard.writeText(target.innerText);
        button.textContent = 'Copied';
      } catch {
        const range = document.createRange();
        range.selectNodeContents(target);
        const selection = window.getSelection();
        selection.removeAllRanges();
        selection.addRange(range);
        button.textContent = 'Select and copy';
      }
      window.setTimeout(() => { button.textContent = original; }, 1600);
    });
  });

  const tabs = document.querySelectorAll('[data-example-tab]');
  const panels = document.querySelectorAll('[data-example-panel]');
  tabs.forEach((tab) => {
    tab.addEventListener('click', () => {
      tabs.forEach((item) => item.setAttribute('aria-selected', String(item === tab)));
      panels.forEach((panel) => {
        panel.hidden = panel.dataset.examplePanel !== tab.dataset.exampleTab;
      });
    });
  });
})();