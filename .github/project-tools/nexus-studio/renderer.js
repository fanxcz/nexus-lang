(() => {
  const api = window.nexusStudio;
  const $ = id => document.getElementById(id);
  let root = '';
  let currentPath = '';
  let dirty = false;
  let editor = null;
  let compilerPath = null;
  let currentUri = null;
  let lspVersion = 0;
  let languageServerReady = false;
  let languageServerCrashed = false;
  let languageServerPromise = null;
  let languageServerGeneration = 0;
  let pendingLanguageSync = Promise.resolve();
  let documentOperationQueue = Promise.resolve();
  let suppressEditorChange = false;
  const documents = new Map();
  const welcome = $('welcome');
  const editorWrap = $('editorWrap');
  const output = $('terminalOutput');
  const treeEl = $('fileTree');
  const tabsEl = $('openTabs');

  function setStatus(text) { $('saveStatus').textContent = text; }
  function appendOutput(text, type = 'stdout') {
    const line = document.createElement('div');
    line.className = 'output-line ' + type;
    line.textContent = String(text);
    output.appendChild(line);
    output.scrollTop = output.scrollHeight;
  }
  function renderTabs() {
    tabsEl.replaceChildren();
    for (const [filePath, item] of documents.entries()) {
      const tab = document.createElement('div');
      tab.className = 'open-tab' + (filePath === currentPath ? ' active' : '');

      const select = document.createElement('button');
      select.type = 'button';
      select.className = 'open-tab-select';
      select.setAttribute('role', 'tab');
      select.setAttribute('aria-selected', String(filePath === currentPath));
      select.title = filePath;
      const fileName = filePath.split(/[\\/]/).pop() || filePath;
      select.textContent = (item.dirty ? '● ' : '') + fileName;
      select.addEventListener('click', () => { void switchDocument(filePath); });

      const close = document.createElement('button');
      close.type = 'button';
      close.className = 'open-tab-close';
      close.textContent = '×';
      close.title = 'Close ' + filePath;
      close.setAttribute('aria-label', 'Close ' + filePath);
      close.addEventListener('click', event => {
        event.stopPropagation();
        void closeDocument(filePath);
      });

      tab.append(select, close);
      tabsEl.appendChild(tab);
    }
  }

  function markDirty(value) {
    dirty = value;
    if (currentPath && documents.has(currentPath)) {
      const active = documents.get(currentPath);
      active.dirty = value;
      if (editor) active.content = editor.getValue();
      if (value) active.revision = (active.revision || 0) + 1;
    }
    $('dirtyMark').textContent = dirty ? '●' : '';
    $('dirtyMark').className = dirty ? 'dirty' : '';
    setStatus(dirty ? 'Unsaved changes' : 'All changes saved');
    renderTabs();
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
    editor.session.on('change', () => {
      if (!currentPath || suppressEditorChange) return;
      markDirty(true);
      if (!currentUri && languageServerCrashed &&
          currentPath.toLowerCase().endsWith('.nx')) {
        const filePath = currentPath;
        languageServerCrashed = false;
        void ensureLanguageServer().then(ready => {
          if (ready && currentPath === filePath && !currentUri) {
            return openCurrentLanguageDocument();
          }
          return false;
        }).catch(error => {
          appendOutput('Language services restart failed: ' + error.message, 'error');
        });
      }
      if (currentUri && languageServerReady) {
        lspVersion += 1;
        const version = lspVersion;
        const filePath = currentPath;
        const source = editor.getValue();
        const active = documents.get(filePath);
        if (active) active.version = version;
        pendingLanguageSync = pendingLanguageSync
          .catch(() => {})
          .then(() => api.changeLanguageDocument(filePath, source, version))
          .catch(error => {
            appendOutput('Language sync failed: ' + error.message, 'error');
          });
      }
    });
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
  async function ensureLanguageServer() {
    if (languageServerReady) return true;
    if (!root) return false;
    if (!languageServerPromise) {
      const generation = languageServerGeneration;
      const pending = api.startLanguageServer()
        .then(() => {
          if (generation !== languageServerGeneration) return false;
          languageServerReady = true;
          languageServerCrashed = false;
          appendOutput('NEXUS language services ready.', 'system');
          return true;
        })
        .catch(error => {
          if (generation === languageServerGeneration) {
            languageServerReady = false;
            appendOutput('Language services unavailable: ' + error.message, 'error');
          }
          return false;
        });
      const tracked = pending.finally(() => {
        if (languageServerPromise === tracked) languageServerPromise = null;
      });
      languageServerPromise = tracked;
    }
    return languageServerPromise;
  }

  async function openCurrentLanguageDocument() {
    if (!currentPath || !editor || !currentPath.toLowerCase().endsWith('.nx')) {
      currentUri = null;
      clearDiagnostics();
      return false;
    }
    if (!languageServerReady && !(await ensureLanguageServer())) return false;
    const opened = await api.openLanguageDocument(currentPath, editor.getValue(), 1);
    currentUri = opened.uri;
    lspVersion = 1;
    const active = documents.get(currentPath);
    if (active) {
      active.uri = currentUri;
      active.version = lspVersion;
    }
    $('diagnosticStatus').textContent = 'Checking…';
    return true;
  }

  async function restartLanguageServer() {
    languageServerGeneration += 1;
    languageServerPromise = null;
    languageServerReady = false;
    if (currentUri && currentPath) {
      await pendingLanguageSync.catch(() => {});
      await api.closeLanguageDocument(currentPath).catch(() => {});
    }
    currentUri = null;
    if (!(await ensureLanguageServer())) return false;
    if (currentPath && currentPath.toLowerCase().endsWith('.nx')) {
      try {
        await openCurrentLanguageDocument();
      } catch (error) {
        appendOutput('Could not reopen language document: ' + error.message, 'error');
      }
    }
    return true;
  }

  function clearDiagnostics() {
    if (editor) editor.session.setAnnotations([]);
    $('diagnosticStatus').textContent = 'No diagnostics';
    $('diagnosticStatus').title = 'NEXUS language diagnostics';
  }

  api.onLanguageServerExit(() => {
    if (languageServerReady) languageServerCrashed = true;
    languageServerReady = false;
    currentUri = null;
    const active = documents.get(currentPath);
    if (active) active.uri = null;
    $('diagnosticStatus').textContent = 'Language server stopped';
    $('diagnosticStatus').title = 'Edit the active .nx file to try restarting language services';
  });

  api.onDiagnostics(params => {
    if (!editor || !currentUri || params.uri !== currentUri) return;
    const diagnostics = Array.isArray(params.diagnostics) ? params.diagnostics : [];
    const annotations = diagnostics.map(diagnostic => {
      const start = diagnostic.range && diagnostic.range.start
        ? diagnostic.range.start : { line: 0, character: 0 };
      const severity = Number(diagnostic.severity) || 1;
      const type = severity === 1 ? 'error' :
        severity === 2 ? 'warning' : severity === 3 ? 'info' : 'info';
      const source = diagnostic.source ? '[' + diagnostic.source + '] ' : '';
      return {
        row: Math.max(0, Number(start.line) || 0),
        column: Math.max(0, Number(start.character) || 0),
        text: source + String(diagnostic.message || 'Language diagnostic'),
        type
      };
    });
    editor.session.setAnnotations(annotations);
    const errors = diagnostics.filter(item => item.severity === 1).length;
    const warnings = diagnostics.filter(item => item.severity === 2).length;
    $('diagnosticStatus').textContent =
      errors + ' errors · ' + warnings + ' warnings';
    $('diagnosticStatus').title = diagnostics.length
      ? diagnostics.map(item => item.message).join('\n')
      : 'No language diagnostics';
  });

  async function refreshTree() {
    if (!root) { appendOutput('Open a project folder first.', 'error'); return; }
    try {
      const items = await api.refreshTree();
      treeEl.replaceChildren();
      if (!items.length) treeEl.innerHTML = '<div class="empty-state">This folder is empty.</div>';
      else renderTree(items, treeEl);
    } catch (error) { appendOutput(error.message, 'error'); }
  }
  function enqueueDocumentOperation(operation) {
    const result = documentOperationQueue.catch(() => {}).then(operation);
    documentOperationQueue = result.catch(() => {});
    return result;
  }

  async function openProjectNow() {
    const hasUnsavedDocuments = Array.from(documents.values()).some(item => item.dirty);
    if (hasUnsavedDocuments &&
        !confirm('Discard unsaved changes in all open documents and open another project?')) return;
    try {
      await pendingLanguageSync.catch(() => {});
      const project = await api.openProject();
      if (!project) return;
      root = project.root;
      currentPath = '';
      currentUri = null;
      lspVersion = 0;
      documents.clear();
      renderTabs();
      languageServerGeneration += 1;
      languageServerPromise = null;
      languageServerReady = false;
      languageServerCrashed = false;
      clearDiagnostics();
      $('projectName').textContent = root.split(/[\\/]/).filter(Boolean).pop() || root;
      $('statusProject').textContent = root;
      treeEl.replaceChildren();
      renderTree(project.tree, treeEl);
      welcome.classList.remove('hidden');
      editorWrap.classList.add('hidden');
      $('tabName').textContent = 'Welcome';
      markDirty(false);
      setStatus('Project opened');
      appendOutput('Opened: ' + root, 'system');
      void ensureLanguageServer();
    } catch (error) { appendOutput('Open folder failed: ' + error.message, 'error'); }
  }

  function openProject() {
    return enqueueDocumentOperation(() => openProjectNow());
  }

  async function switchDocumentNow(rel) {
    const next = documents.get(rel);
    if (!next) return false;
    if (currentPath === rel && editor) return true;

    if (currentPath && editor && documents.has(currentPath)) {
      const previous = documents.get(currentPath);
      previous.content = editor.getValue();
      previous.dirty = dirty;
      previous.uri = currentUri;
    }
    if (currentUri && currentPath) {
      await pendingLanguageSync.catch(() => {});
      await api.closeLanguageDocument(currentPath).catch(() => {});
      const previous = documents.get(currentPath);
      if (previous) previous.uri = null;
    }

    currentUri = null;
    lspVersion = 0;
    clearDiagnostics();
    ensureEditor();
    if (!editor) return false;

    currentPath = rel;
    suppressEditorChange = true;
    editor.setValue(next.content, -1);
    suppressEditorChange = false;
    editor.session.setMode(rel.toLowerCase().endsWith('.nx') ? 'ace/mode/nexus' : 'ace/mode/text');
    welcome.classList.add('hidden');
    editorWrap.classList.remove('hidden');
    $('tabName').textContent = rel.split(/[\\/]/).pop() || rel;
    $('activeTab').title = rel;
    $('statusLang').textContent = rel.toLowerCase().endsWith('.nx') ? 'NEXUS' : 'TEXT';
    markDirty(!!next.dirty);
    await refreshTree();

    if (rel.toLowerCase().endsWith('.nx') && await ensureLanguageServer()) {
      try {
        await openCurrentLanguageDocument();
      } catch (error) {
        appendOutput('Could not open language document: ' + error.message, 'error');
      }
    } else {
      clearDiagnostics();
    }
    setStatus('Opened ' + rel);
    return true;
  }

  function switchDocument(rel) {
    return enqueueDocumentOperation(() => switchDocumentNow(rel));
  }

  async function openFileNow(rel) {
    try {
      if (!documents.has(rel)) {
        const content = await api.readFile(rel);
        documents.set(rel, {
          path: rel,
          content,
          dirty: false,
          uri: null,
          version: 0,
          revision: 0
        });
        renderTabs();
      }
      const opened = await switchDocumentNow(rel);
      if (!opened && documents.get(rel)?.dirty === false) {
        documents.delete(rel);
        renderTabs();
      }
      return opened;
    } catch (error) {
      appendOutput('Open file failed: ' + error.message, 'error');
      return false;
    }
  }

  function openFile(rel) {
    return enqueueDocumentOperation(() => openFileNow(rel));
  }

  async function closeDocumentNow(rel) {
    const item = documents.get(rel);
    if (!item) return false;
    if (item.dirty && !confirm('Discard unsaved changes in ' + rel + '?')) return false;

    if (rel === currentPath) {
      if (currentUri) {
        await pendingLanguageSync.catch(() => {});
        await api.closeLanguageDocument(rel).catch(() => {});
      }
      documents.delete(rel);
      currentUri = null;
      currentPath = '';
      lspVersion = 0;
      if (documents.size) {
        const nextPath = Array.from(documents.keys()).pop();
        await switchDocumentNow(nextPath);
      } else {
        clearDiagnostics();
        if (editor) {
          suppressEditorChange = true;
          editor.setValue('', -1);
          suppressEditorChange = false;
        }
        welcome.classList.remove('hidden');
        editorWrap.classList.add('hidden');
        $('tabName').textContent = 'Welcome';
        $('activeTab').title = '';
        $('statusLang').textContent = 'NEXUS';
        markDirty(false);
        await refreshTree();
      }
    } else {
      documents.delete(rel);
      renderTabs();
    }
    return true;
  }

  function closeDocument(rel) {
    return enqueueDocumentOperation(() => closeDocumentNow(rel));
  }

  async function saveCurrentNow() {
    if (!currentPath || !editor) {
      appendOutput('Open or create a file before saving.', 'error');
      return false;
    }
    const filePath = currentPath;
    const content = editor.getValue();
    const savedDocument = documents.get(filePath);
    const revisionAtSave = savedDocument ? (savedDocument.revision || 0) : 0;
    try {
      await api.saveFile(filePath, content);
      await pendingLanguageSync.catch(() => {});
      if (currentUri && currentPath === filePath && languageServerReady) {
        await api.saveLanguageDocument(filePath);
      }
      const saved = documents.get(filePath);
      const noNewerEdits = !saved || (saved.revision || 0) === revisionAtSave;
      if (saved && noNewerEdits) {
        saved.content = content;
        saved.dirty = false;
      }
      if (currentPath === filePath && noNewerEdits) markDirty(false);
      else renderTabs();
      setStatus(noNewerEdits
        ? 'Saved ' + filePath
        : 'Saved snapshot; newer edits remain unsaved in ' + filePath);
      await refreshTree();
      return true;
    } catch (error) {
      appendOutput('Save failed: ' + error.message, 'error');
      return false;
    }
  }

  function saveCurrent() {
    return enqueueDocumentOperation(() => saveCurrentNow());
  }
  async function newFileNow() {
    if (!root) { await openProjectNow(); if (!root) return; }
    const rel = prompt('File path relative to the project folder:', 'main.nx');
    if (!rel) return;
    if (rel.startsWith('/') || rel.split(/[\\/]/).includes('..') || /^[A-Za-z]:/.test(rel)) {
      appendOutput('Enter a relative path inside the project folder.', 'error');
      return;
    }
    try {
      await api.newFile(rel);
      await refreshTree();
      await openFileNow(rel);
    } catch (error) { appendOutput('Create file failed: ' + error.message, 'error'); }
  }

  function newFile() {
    return enqueueDocumentOperation(() => newFileNow());
  }
  async function chooseCompiler() {
    try {
      const result = await api.chooseCompiler();
      if (!result) return;
      compilerPath = result.path;
      updateCompilerStatus();
      appendOutput('Compiler selected: ' + compilerPath, 'system');
      if (root) await restartLanguageServer();
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
  async function buildOrRunNow(mode) {
    if (!root) { await openProjectNow(); if (!root) return; }
    if (dirty && !(await saveCurrentNow())) return;
    if (!currentPath || !currentPath.toLowerCase().endsWith('.nx')) {
      try {
        const selected = await api.chooseSource();
        if (!selected) return;
        const opened = await openFileNow(selected);
        if (!opened || currentPath !== selected) return;
        if (dirty && !(await saveCurrentNow())) return;
      } catch (error) {
        appendOutput('Select source failed: ' + error.message, 'error');
        return;
      }
    }
    if (!currentPath || !currentPath.toLowerCase().endsWith('.nx')) {
      appendOutput('Select a .nx source file before building or running.', 'error');
      return;
    }
    const sourcePath = currentPath;
    if (!(await ensureCompiler())) return;
    try {
      setStatus(mode === 'build' ? 'Building…' : 'Running…');
      appendOutput((mode === 'build' ? 'Build started: ' : 'Run started: ') + sourcePath, 'system');
      if (mode === 'build') await api.buildFile(sourcePath);
      else await api.runFile(sourcePath);
    } catch (error) {
      appendOutput((mode === 'build' ? 'Build failed: ' : 'Run failed: ') + error.message, 'error');
      setStatus('Failed');
    }
  }

  function buildOrRun(mode) {
    return enqueueDocumentOperation(() => buildOrRunNow(mode));
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
  $('toggleTerminal').addEventListener('click', () => {
    const panel = document.querySelector('.terminal-panel');
    const collapsed = panel.classList.toggle('collapsed');
    $('toggleTerminal').textContent = collapsed ? 'Show' : 'Hide';
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
      void ensureLanguageServer();
    }
  }).catch(() => {});
})();
