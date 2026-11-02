(() => {
  const api = window.nexusStudio;
  const $ = id => document.getElementById(id);
  let root = '';
  let currentPath = '';
  let dirty = false;
  let editor = null;
  let compilerPath = null;
  const welcome = $('welcome');
  const editorWrap = $('editorWrap');
  const output = $('terminalOutput');
  const treeEl = $('fileTree');

  function setStatus(text) { $('saveStatus').textContent = text; }
  function appendOutput(text, type = 'stdout') {
    const line = document.createElement('div');
    line.className = 'output-line ' + type;
    line.textContent = String(text);
    output.appendChild(line);
    output.scrollTop = output.scrollHeight;
  }
  function markDirty(value) {
    dirty = value;
    $('dirtyMark').textContent = dirty ? '●' : '';
    $('dirtyMark').className = dirty ? 'dirty' : '';
    setStatus(dirty ? 'Unsaved changes' : 'All changes saved');
  }
  function ensureEditor() {
    if (editor) return;
    if (!window.ace) {
      appendOutput('The editor library could not be loaded. Reinstall NEXUS Studio dependencies.', 'error');
      return;
    }
    editor = ace.edit('editor');
    editor.setTheme('ace/theme/nexus');
    editor.session.setMode('ace/mode/nexus');
    editor.setOptions({
      fontSize: '14px',
      showPrintMargin: false,
      tabSize: 4,
      useSoftTabs: true,
      wrap: false,
      highlightActiveLine: true
    });
    editor.session.on('change', () => { if (currentPath) markDirty(true); });
    editor.commands.addCommand({
      name: 'save',
      bindKey: { win: 'Ctrl-S', mac: 'Command-S' },
      exec: saveCurrent
    });
  }
  function renderTree(items, container, depth = 0) {
    for (const item of items) {
      const row = document.createElement('button');
      row.className = 'tree-item' + (item.path === currentPath ? ' active' : '');
      row.style.paddingLeft = (10 + depth * 14) + 'px';
      row.title = item.path;
      const glyph = document.createElement('span');
      glyph.className = item.directory ? 'tree-folder' : 'tree-file';
      glyph.textContent = item.directory ? '▸' : (item.name.endsWith('.nx') ? '◇' : '·');
      const label = document.createElement('span');
      label.className = 'tree-label';
      label.textContent = item.name;
      row.append(glyph, label);
      if (item.directory) {
        row.addEventListener('click', () => {
          const next = row.nextElementSibling;
          if (next && next.classList.contains('tree-children')) {
            next.remove();
            glyph.textContent = '▸';
          } else {
            const child = document.createElement('div');
            child.className = 'tree-children';
            renderTree(item.children || [], child, depth + 1);
            row.after(child);
            glyph.textContent = '▾';
          }
        });
      } else {
        row.addEventListener('click', () => openFile(item.path));
      }
      container.appendChild(row);
    }
  }
  async function refreshTree() {
    if (!root) { appendOutput('Open a project folder first.', 'error'); return; }
    try {
      const items = await api.refreshTree();
      treeEl.replaceChildren();
      if (!items.length) treeEl.innerHTML = '<div class="empty-state">This folder is empty.</div>';
      else renderTree(items, treeEl);
    } catch (error) { appendOutput(error.message, 'error'); }
  }
  async function openProject() {
    if (dirty && !confirm('Discard unsaved changes and open another project?')) return;
    try {
      const project = await api.openProject();
      if (!project) return;
      root = project.root;
      currentPath = '';
      $('projectName').textContent = root.split(/[\\/]/).filter(Boolean).pop() || root;
      $('projectNameSidebar').textContent = root.split(/[\\/]/).filter(Boolean).pop() || root;
      $('breadcrumbRoot').textContent = root.split(/[\\/]/).filter(Boolean).pop() || 'NEXUS';
      $('breadcrumbCurrent').textContent = 'Welcome';
      $('editorLanguage').textContent = 'WORKSPACE';
      $('statusProject').textContent = root;
      treeEl.replaceChildren();
      renderTree(project.tree, treeEl);
      welcome.classList.remove('hidden');
      editorWrap.classList.add('hidden');
      $('tabName').textContent = 'Welcome';
      markDirty(false);
      setStatus('Project opened');
      appendOutput('Opened: ' + root, 'system');
    } catch (error) { appendOutput('Open folder failed: ' + error.message, 'error'); }
  }
  async function openFile(rel) {
    if (dirty && !confirm('Discard unsaved changes?')) return false;
    try {
      const content = await api.readFile(rel);
      ensureEditor();
      if (!editor) return;
      currentPath = rel;
      editor.setValue(content, -1);
      editor.session.setMode(rel.toLowerCase().endsWith('.nx') ? 'ace/mode/nexus' : 'ace/mode/text');
      welcome.classList.add('hidden');
      editorWrap.classList.remove('hidden');
      $('tabName').textContent = rel.split('/').pop();
      $('breadcrumbRoot').textContent = root.split(/[\\/]/).filter(Boolean).pop() || 'NEXUS';
      $('breadcrumbCurrent').textContent = rel;
      $('editorLanguage').textContent = rel.toLowerCase().endsWith('.nx') ? 'NEXUS SOURCE' : 'TEXT FILE';
      $('activeTab').title = rel;
      $('statusLang').textContent = rel.toLowerCase().endsWith('.nx') ? 'NEXUS' : 'TEXT';
      markDirty(false);
      await refreshTree();
      setStatus('Opened ' + rel);
      return true;
    } catch (error) {
      appendOutput('Open file failed: ' + error.message, 'error');
      return false;
    }
  }
  async function saveCurrent() {
    if (!currentPath || !editor) {
      appendOutput('Open or create a file before saving.', 'error');
      return false;
    }
    try {
      await api.saveFile(currentPath, editor.getValue());
      markDirty(false);
      setStatus('Saved ' + currentPath);
      await refreshTree();
      return true;
    } catch (error) {
      appendOutput('Save failed: ' + error.message, 'error');
      return false;
    }
  }
  async function newFile() {
    if (!root) { await openProject(); if (!root) return; }
    if (dirty && !confirm('Discard unsaved changes and create another file?')) return;
    const rel = prompt('File path relative to the project folder:', 'main.nx');
    if (!rel) return;
    if (rel.startsWith('/') || rel.split(/[\\/]/).includes('..') || /^[A-Za-z]:/.test(rel)) {
      appendOutput('Enter a relative path inside the project folder.', 'error');
      return;
    }
    try {
      await api.newFile(rel);
      await refreshTree();
      await openFile(rel);
    } catch (error) { appendOutput('Create file failed: ' + error.message, 'error'); }
  }
  async function chooseCompiler() {
    try {
      const result = await api.chooseCompiler();
      if (!result) return;
      compilerPath = result.path;
      updateCompilerStatus();
      appendOutput('Compiler selected: ' + compilerPath, 'system');
    } catch (error) { appendOutput('Compiler selection failed: ' + error.message, 'error'); }
  }
  function updateCompilerStatus() {
    const element = $('compilerStatus');
    if (compilerPath) {
      element.textContent = 'Compiler: ' + compilerPath.split(/[\\/]/).pop();
      element.title = compilerPath;
      element.className = 'compiler-ready';
    } else {
      element.textContent = 'Compiler not selected';
      element.title = 'Click Compiler… to select the installed NEXUS compiler';
      element.className = 'compiler-missing';
    }
  }
  async function ensureCompiler() {
    const info = await api.compilerInfo();
    compilerPath = info.path || null;
    updateCompilerStatus();
    if (compilerPath) return true;
    appendOutput('NEXUS compiler was not found. Select nexus.cmd from the installed NEXUS folder or nexus-core.exe from its bin folder.', 'error');
    await chooseCompiler();
    return !!compilerPath;
  }
  async function buildOrRun(mode) {
    if (!root) { await openProject(); if (!root) return; }
    if (dirty && !(await saveCurrent())) return;
    if (!currentPath || !currentPath.toLowerCase().endsWith('.nx')) {
      try {
        const selected = await api.chooseSource();
        if (!selected) return;
        const opened = await openFile(selected);
        if (!opened || currentPath !== selected) return;
        if (dirty && !(await saveCurrent())) return;
      } catch (error) {
        appendOutput('Select source failed: ' + error.message, 'error');
        return;
      }
    }
    if (!currentPath || !currentPath.toLowerCase().endsWith('.nx')) {
      appendOutput('Select a .nx source file before building or running.', 'error');
      return;
    }
    if (!(await ensureCompiler())) return;
    try {
      setStatus(mode === 'build' ? 'Building…' : 'Running…');
      appendOutput((mode === 'build' ? 'Build started: ' : 'Run started: ') + currentPath, 'system');
      if (mode === 'build') await api.buildFile(currentPath);
      else await api.runFile(currentPath);
    } catch (error) {
      appendOutput((mode === 'build' ? 'Build failed: ' : 'Run failed: ') + error.message, 'error');
      setStatus('Failed');
    }
  }

  const commandOverlay = $('commandOverlay');
  const commandSearch = $('commandSearch');
  const commandList = $('commandList');
  const paletteCommands = [
    { label: 'Open project folder', detail: 'Choose a workspace from your computer', icon: '⌂', shortcut: 'Ctrl O', action: openProject },
    { label: 'Create a source file', detail: 'Add a new .nx file to the current workspace', icon: '＋', shortcut: 'Ctrl N', action: newFile },
    { label: 'Save current file', detail: 'Write the active editor buffer to disk', icon: '↧', shortcut: 'Ctrl S', action: saveCurrent },
    { label: 'Select NEXUS compiler', detail: 'Choose the compiler executable or launcher', icon: '⌘', shortcut: '', action: chooseCompiler },
    { label: 'Build current file', detail: 'Compile the active NEXUS source file', icon: '↗', shortcut: 'F7', action: () => buildOrRun('build') },
    { label: 'Build and run current file', detail: 'Compile, then launch the generated program', icon: '▶', shortcut: 'F5', action: () => buildOrRun('run') },
    { label: 'Refresh project explorer', detail: 'Rescan files in the active workspace', icon: '↻', shortcut: '', action: refreshTree },
    { label: 'Toggle terminal panel', detail: 'Show or hide the integrated terminal', icon: '›_', shortcut: '', action: toggleTerminalPanel }
  ];
  let visibleCommands = paletteCommands.slice();
  let selectedCommandIndex = 0;

  function renderCommandList() {
    const query = commandSearch.value.trim().toLowerCase();
    visibleCommands = paletteCommands.filter(command =>
      (command.label + ' ' + command.detail).toLowerCase().includes(query)
    );
    selectedCommandIndex = Math.min(selectedCommandIndex, Math.max(0, visibleCommands.length - 1));
    commandList.replaceChildren();
    if (!visibleCommands.length) {
      const empty = document.createElement('div');
      empty.className = 'command-empty';
      empty.textContent = 'No matching commands.';
      commandList.appendChild(empty);
      return;
    }
    visibleCommands.forEach((command, index) => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'command-option' + (index === selectedCommandIndex ? ' selected' : '');
      row.setAttribute('role', 'option');
      row.setAttribute('aria-selected', index === selectedCommandIndex ? 'true' : 'false');
      const icon = document.createElement('span');
      icon.className = 'command-option-icon';
      icon.textContent = command.icon;
      const copy = document.createElement('span');
      copy.className = 'command-option-copy';
      const title = document.createElement('strong');
      title.textContent = command.label;
      const detail = document.createElement('small');
      detail.textContent = command.detail;
      copy.append(title, detail);
      row.append(icon, copy);
      if (command.shortcut) {
        const shortcut = document.createElement('kbd');
        shortcut.textContent = command.shortcut;
        row.appendChild(shortcut);
      }
      row.addEventListener('mouseenter', () => {
        selectedCommandIndex = index;
        updateCommandSelection();
      });
      row.addEventListener('click', () => executePaletteCommand(index));
      commandList.appendChild(row);
    });
  }

  function updateCommandSelection() {
    [...commandList.querySelectorAll('.command-option')].forEach((row, index) => {
      const active = index === selectedCommandIndex;
      row.classList.toggle('selected', active);
      row.setAttribute('aria-selected', active ? 'true' : 'false');
    });
    const active = commandList.querySelectorAll('.command-option')[selectedCommandIndex];
    if (active) active.scrollIntoView({ block: 'nearest' });
  }

  function openCommandPalette() {
    commandOverlay.classList.remove('hidden');
    commandOverlay.setAttribute('aria-hidden', 'false');
    $('openCommands').setAttribute('aria-expanded', 'true');
    commandSearch.value = '';
    selectedCommandIndex = 0;
    renderCommandList();
    commandSearch.focus();
  }

  function closeCommandPalette() {
    commandOverlay.classList.add('hidden');
    commandOverlay.setAttribute('aria-hidden', 'true');
    $('openCommands').setAttribute('aria-expanded', 'false');
    $('openCommands').focus();
  }

  function executePaletteCommand(index = selectedCommandIndex) {
    const command = visibleCommands[index];
    if (!command) return;
    closeCommandPalette();
    Promise.resolve().then(() => command.action()).catch(error => {
      appendOutput(command.label + ' failed: ' + error.message, 'error');
    });
  }

  function toggleTerminalPanel() {
    const panel = document.querySelector('.terminal-panel');
    const collapsed = panel.classList.toggle('collapsed');
    $('toggleTerminal').textContent = collapsed ? 'Show' : 'Hide';
  }

  async function runTerminalCommand(command) {
    if (!root) { appendOutput('Open a project folder first.', 'error'); return; }
    appendOutput('> ' + command, 'command');
    try { await api.runCommand(command); }
    catch (error) { appendOutput(error.message, 'error'); }
  }

  api.onTerminalData(({ type, data }) => {
    if (type === 'exit') {
      appendOutput('[' + data + ']', 'system');
      setStatus('Ready');
    } else {
      appendOutput(data, type === 'stderr' || type === 'error' ? 'error' : type);
    }
  });

  $('openProject').addEventListener('click', openProject);
  $('welcomeOpen').addEventListener('click', openProject);
  $('newFile').addEventListener('click', newFile);
  $('welcomeNew').addEventListener('click', newFile);
  $('saveFile').addEventListener('click', saveCurrent);
  $('selectCompiler').addEventListener('click', chooseCompiler);
  $('buildFile').addEventListener('click', () => buildOrRun('build'));
  $('runFile').addEventListener('click', () => buildOrRun('run'));
  $('refreshTree').addEventListener('click', refreshTree);
  $('clearTerminal').addEventListener('click', () => {
    output.replaceChildren();
    appendOutput('Output cleared.', 'system');
  });
  $('stopTerminal').addEventListener('click', async () => {
    try {
      const stopped = await api.stopCommand();
      appendOutput(stopped ? 'Stop requested.' : 'No command is running.', 'system');
    } catch (error) { appendOutput('Stop failed: ' + error.message, 'error'); }
  });
  $('toggleTerminal').addEventListener('click', toggleTerminalPanel);
  $('openCommands').addEventListener('click', openCommandPalette);
  $('sidebarOpen').addEventListener('click', openProject);
  $('sidebarCompiler').addEventListener('click', event => {
    event.preventDefault();
    chooseCompiler();
  });
  commandSearch.addEventListener('input', () => {
    selectedCommandIndex = 0;
    renderCommandList();
  });
  commandOverlay.addEventListener('mousedown', event => {
    if (event.target === commandOverlay) closeCommandPalette();
  });
  commandSearch.addEventListener('keydown', event => {
    if (event.key === 'ArrowDown') {
      event.preventDefault();
      selectedCommandIndex = Math.min(selectedCommandIndex + 1, visibleCommands.length - 1);
      updateCommandSelection();
    } else if (event.key === 'ArrowUp') {
      event.preventDefault();
      selectedCommandIndex = Math.max(0, selectedCommandIndex - 1);
      updateCommandSelection();
    } else if (event.key === 'Enter') {
      event.preventDefault();
      executePaletteCommand();
    } else if (event.key === 'Escape') {
      event.preventDefault();
      closeCommandPalette();
    }
  });
  $('terminalForm').addEventListener('submit', async event => {
    event.preventDefault();
    const input = $('terminalInput');
    const command = input.value.trim();
    if (!command) return;
    input.value = '';
    await runTerminalCommand(command);
  });
  document.addEventListener('keydown', event => {
    if ((event.ctrlKey || event.metaKey) && event.shiftKey && event.key.toLowerCase() === 'p') {
      event.preventDefault();
      openCommandPalette();
      return;
    }
    if (!commandOverlay.classList.contains('hidden')) {
      if (event.key === 'Escape') { event.preventDefault(); closeCommandPalette(); }
      return;
    }
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'o') {
      event.preventDefault();
      openProject();
    }
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 's') {
      event.preventDefault();
      saveCurrent();
    }
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'n') {
      event.preventDefault();
      newFile();
    }
    if (event.key === 'F7') {
      event.preventDefault();
      buildOrRun('build');
    }
    if (event.key === 'F5') {
      event.preventDefault();
      buildOrRun('run');
    }
  });

  api.platform().then(platform => {
    $('statusPlatform').textContent = ({ win32: 'Windows', darwin: 'macOS', linux: 'Linux' })[platform] || platform;
  });
  api.compilerInfo().then(info => {
    compilerPath = info.path || null;
    updateCompilerStatus();
  }).catch(() => updateCompilerStatus());
  api.currentProject().then(project => {
    if (project.root) {
      root = project.root;
      $('projectName').textContent = root.split(/[\\/]/).filter(Boolean).pop() || root;
      $('statusProject').textContent = root;
      treeEl.replaceChildren();
      renderTree(project.tree || [], treeEl);
    }
  }).catch(() => {});
})();
