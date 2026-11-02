(() => {
  const api = window.nexusStudio, $ = id => document.getElementById(id);
  let root = '', currentPath = '', editor = null, compilerPath = null, treeData = [], tabs = [], problems = [];
  const buffers = new Map(), welcome = $('welcome'), editorWrap = $('editorWrap'), output = $('terminalOutput'), treeEl = $('fileTree');
  const isDirty = path => { const b = buffers.get(path); return !!b && b.content !== b.saved; };
  const dirtyAny = () => tabs.some(isDirty);
  const projectName = () => root.split(/[\\/]/).filter(Boolean).pop() || root || 'NEXUS';
  function status(value) { $('saveStatus').textContent = value; }
  function renderTabs() {
    const host = $('editorTabs'); host.replaceChildren();
    tabs.forEach(path => {
      const tab = document.createElement('div');
      tab.className = 'tab' + (path === currentPath ? ' active' : '') + (isDirty(path) ? ' is-dirty' : '');
      const activate = document.createElement('button'); activate.type = 'button'; activate.className = 'tab-main'; activate.setAttribute('role','tab'); activate.setAttribute('aria-selected', String(path === currentPath));
      const glyph = document.createElement('span'); glyph.className = 'file-glyph'; glyph.textContent = path.toLowerCase().endsWith('.nx') ? '◇' : '·';
      const name = document.createElement('span'); name.className = 'tab-label'; name.textContent = path.split(/[\\/]/).pop();
      const dot = document.createElement('span'); dot.className = 'tab-dirty-dot'; dot.textContent = isDirty(path) ? '●' : '';
      activate.append(glyph,name,dot); activate.addEventListener('click', () => openFile(path));
      const close = document.createElement('button'); close.type = 'button'; close.className = 'tab-close'; close.textContent = '×'; close.title = 'Close ' + path; close.setAttribute('aria-label','Close ' + path);
      close.addEventListener('click', e => { e.stopPropagation(); closeTab(path); }); tab.append(activate,close); host.appendChild(tab);
    });
  }
  function markDirty() {
    const changed = isDirty(currentPath);
    $('dirtyMark').textContent = changed ? '●' : '';
    status(changed ? 'Unsaved changes' : 'All changes saved');
    renderTabs();
  }
  function syncBuffer() { if (editor && currentPath && buffers.has(currentPath)) buffers.get(currentPath).content = editor.getValue(); }
  function cursorPosition() {
    if (!editor) return;
    const p = editor.getCursorPosition();
    $('cursorPosition').textContent = 'Ln ' + (p.row + 1) + ', Col ' + (p.column + 1);
  }
  function ensureEditor() {
    if (editor) return;
    if (!window.ace) { appendOutput('Ace editor could not be loaded. Reinstall NEXUS Studio dependencies.', 'error'); return; }
    editor = ace.edit('editor'); editor.setTheme('ace/theme/nexus'); editor.session.setMode('ace/mode/nexus');
    editor.setOptions({fontSize:'14px',showPrintMargin:false,tabSize:4,useSoftTabs:true,wrap:false,highlightActiveLine:true,showLineNumbers:true,showGutter:true,scrollPastEnd:0.35});
    editor.session.on('change', () => { if (currentPath && buffers.has(currentPath)) { buffers.get(currentPath).content = editor.getValue(); markDirty(); } });
    editor.selection.on('changeCursor',cursorPosition);
    editor.commands.addCommand({name:'save',bindKey:{win:'Ctrl-S',mac:'Command-S'},exec:saveCurrent});
    editor.commands.addCommand({name:'closeTab',bindKey:{win:'Ctrl-W',mac:'Command-W'},exec:()=>closeTab(currentPath)});
  }
  function appendOutput(value,type='stdout') {
    const line = document.createElement('div'); line.className = 'output-line ' + type; line.textContent = String(value); output.appendChild(line); output.scrollTop = output.scrollHeight;
    if (type === 'stderr' || type === 'error') String(value).split(/\r?\n/).forEach(parseDiagnostic);
  }
  function filtered(items,query) {
    if (!query) return items;
    return items.map(item => {
      if (!item.directory) return item.name.toLowerCase().includes(query) ? item : null;
      const children = filtered(item.children || [],query);
      return children.length ? {...item,children} : null;
    }).filter(Boolean);
  }
  function renderTree(items,host,depth=0,expanded=false) {
    items.forEach(item => {
      const row = document.createElement('button'); row.type='button'; row.className='tree-item'+(item.path===currentPath?' active':''); row.style.paddingLeft=(10+depth*14)+'px'; row.title=item.path;
      const glyph=document.createElement('span'); glyph.className=item.directory?'tree-folder':'tree-file'; glyph.textContent=item.directory?(expanded?'▾':'▸'):(item.name.endsWith('.nx')?'◇':'·');
      const label=document.createElement('span'); label.className='tree-label'; label.textContent=item.name; row.append(glyph,label); host.appendChild(row);
      if(item.directory) row.addEventListener('click',()=>{
        const next=row.nextElementSibling;
        if(next&&next.classList.contains('tree-children')){next.remove();glyph.textContent='▸';}
        else{const child=document.createElement('div');child.className='tree-children';renderTree(item.children||[],child,depth+1,false);row.after(child);glyph.textContent='▾';}
      });
      else row.addEventListener('click',()=>openFile(item.path));
      if(item.directory&&expanded){const child=document.createElement('div');child.className='tree-children';renderTree(item.children||[],child,depth+1,true);host.appendChild(child);}
    });
  }
  function drawTree() {
    treeEl.replaceChildren(); if(!root) return;
    const query=$('fileSearch').value.trim().toLowerCase(), items=filtered(treeData,query);
    if(!items.length){const empty=document.createElement('div');empty.className='empty-state';empty.textContent=query?'No files match this filter.':'This folder is empty.';treeEl.appendChild(empty);return;}
    renderTree(items,treeEl,0,!!query);
  }
  async function refreshTree() {
    if(!root){appendOutput('Open a project folder first.','error');return;}
    try{treeData=await api.refreshTree();drawTree();}catch(error){appendOutput(error.message,'error');}
  }
  function welcomeScreen() {
    currentPath=''; welcome.classList.remove('hidden'); editorWrap.classList.add('hidden');
    $('tabName').textContent='Welcome'; $('breadcrumbRoot').textContent=projectName(); $('breadcrumbCurrent').textContent='Welcome'; $('editorLanguage').textContent='WORKSPACE';
    $('dirtyMark').textContent=''; $('statusLang').textContent='NEXUS'; $('cursorPosition').textContent='Ln 1, Col 1'; renderTabs(); drawTree(); status('Workspace ready');
  }
  async function openProject() {
    if(dirtyAny()&&!confirm('Discard unsaved editor changes and open another project?')) return;
    try {
      const project=await api.openProject(); if(!project)return;
      root=project.root; treeData=project.tree||[]; buffers.clear(); tabs=[];
      $('projectName').textContent=projectName(); $('projectNameSidebar').textContent=projectName(); $('statusProject').textContent=root; $('fileSearch').value='';
      welcomeScreen(); appendOutput('Opened project: '+root,'system');
    } catch(error){appendOutput('Open folder failed: '+error.message,'error');}
  }
  async function openFile(path) {
    try {
      syncBuffer();
      if(!buffers.has(path)){const content=await api.readFile(path);buffers.set(path,{content,saved:content});}
      if(!tabs.includes(path))tabs.push(path);
      currentPath=path; ensureEditor(); if(!editor)return false;
      editor.setValue(buffers.get(path).content,-1); const nexus=path.toLowerCase().endsWith('.nx');
      editor.session.setMode(nexus?'ace/mode/nexus':'ace/mode/text'); welcome.classList.add('hidden'); editorWrap.classList.remove('hidden');
      $('tabName').textContent=path.split(/[\\/]/).pop(); $('activeTab').title=path; $('breadcrumbRoot').textContent=projectName(); $('breadcrumbCurrent').textContent=path;
      $('editorLanguage').textContent=nexus?'NEXUS SOURCE':'TEXT FILE'; $('statusLang').textContent=nexus?'NEXUS':'TEXT';
      markDirty(); cursorPosition(); renderTabs(); await refreshTree(); editor.focus(); return true;
    } catch(error){appendOutput('Open file failed: '+error.message,'error');return false;}
  }
  async function savePath(path) {
    const b=buffers.get(path); if(!b)return true; if(path===currentPath)syncBuffer(); if(!isDirty(path))return true;
    await api.saveFile(path,b.content); b.saved=b.content; return true;
  }
  async function saveCurrent() {
    if(!currentPath||!editor){appendOutput('Open or create a file before saving.','error');return false;}
    try{await savePath(currentPath);markDirty();status('Saved '+currentPath);await refreshTree();return true;}
    catch(error){appendOutput('Save failed: '+error.message,'error');return false;}
  }
  async function saveAll() {
    try{syncBuffer();for(const path of tabs)await savePath(path);markDirty();status('All open files saved');await refreshTree();}
    catch(error){appendOutput('Save all failed: '+error.message,'error');}
  }
  async function closeTab(path) {
    if(!path||!buffers.has(path))return;
    if(isDirty(path)&&!confirm('Discard unsaved changes in '+path+'?'))return;
    const index=tabs.indexOf(path);tabs=tabs.filter(item=>item!==path);buffers.delete(path);
    if(path===currentPath){const next=tabs[Math.min(Math.max(index-1,0),tabs.length-1)];if(next)await openFile(next);else welcomeScreen();}
    else{renderTabs();markDirty();}
  }
  async function newFile() {
    if(!root){await openProject();if(!root)return;}
    const path=prompt('File path relative to the project folder:','main.nx');if(!path)return;
    if(path.startsWith('/')||path.split(/[\\/]/).includes('..')||/^[A-Za-z]:/.test(path)){appendOutput('Enter a relative path inside the project folder.','error');return;}
    try{await api.newFile(path);await refreshTree();await openFile(path);}
    catch(error){appendOutput('Create file failed: '+error.message,'error');}
  }
  async function chooseCompiler() {
    try{const result=await api.chooseCompiler();if(!result)return;compilerPath=result.path;updateCompilerStatus();appendOutput('Compiler selected: '+compilerPath,'system');}
    catch(error){appendOutput('Compiler selection failed: '+error.message,'error');}
  }
  function updateCompilerStatus() {
    const element=$('compilerStatus');
    if(compilerPath){element.textContent='Compiler: '+compilerPath.split(/[\\/]/).pop();element.title=compilerPath;element.className='compiler-ready';}
    else{element.textContent='Compiler not selected';element.title='Select the installed NEXUS compiler';element.className='compiler-missing';}
  }
  async function ensureCompiler() {
    const info=await api.compilerInfo();compilerPath=info.path||null;updateCompilerStatus();if(compilerPath)return true;
    appendOutput('NEXUS compiler was not found. Select nexus.cmd or nexus-core.exe from your NEXUS installation.','error');await chooseCompiler();return !!compilerPath;
  }
  function parseDiagnostic(raw) {
    const text=String(raw).trim();if(!text)return;
    let m=text.match(/^(.+?):(\d+):(?:(\d+):)?\s*(fatal\s+error|error|warning)(?:\[[^\]]+\])?\s*:?\s*(.*)$/i);
    if(!m)m=text.match(/^(.+?)\((\d+)(?:,(\d+))?\)\s*:\s*(fatal\s+error|error|warning)(?:\[[^\]]+\])?\s*:?\s*(.*)$/i);
    if(!m&&/^(fatal\s+error|error|warning)(?:\[[^\]]+\])?\s*:/i.test(text)){const severity=text.match(/^(fatal\s+error|error|warning)/i)||['error'];m=[text,currentPath||'','1','1',severity[0],text.replace(/^(fatal\s+error|error|warning)(?:\[[^\]]+\])?\s*:\s*/i,'')];}
    if(!m)return;
    const file=String(m[1]||currentPath||'').trim(), severity=String(m[4]).toLowerCase().includes('warning')?'warning':'error';
    const relative=root&&(file.startsWith(root+'/')||file.startsWith(root+'\\'))?file.slice(root.length+1).replace(/\\/g,'/'):file.replace(/\\/g,'/');
    const problem={file:relative,line:Math.max(1,Number(m[2])||1),column:Math.max(1,Number(m[3])||1),severity,message:String(m[5]||text).trim()};
    if(problems.some(p=>p.file===problem.file&&p.line===problem.line&&p.message===problem.message))return;
    problems.push(problem);renderProblems();
  }
  function renderProblems() {
    const host=$('problemList');$('problemCount').textContent=String(problems.length);$('problemBadge').textContent=String(problems.length);
    $('toggleProblems').classList.toggle('has-problems',problems.length>0);host.replaceChildren();
    if(!problems.length){const empty=document.createElement('div');empty.className='problems-empty';empty.textContent='No problems detected. Build a NEXUS file to see diagnostics here.';host.appendChild(empty);return;}
    problems.forEach(problem=>{
      const row=document.createElement('button');row.type='button';row.className='problem-row '+problem.severity;
      const marker=document.createElement('span');marker.className='problem-severity';marker.textContent=problem.severity==='warning'?'▲':'●';
      const copy=document.createElement('span');copy.className='problem-message';const msg=document.createElement('strong');msg.textContent=problem.message;
      const loc=document.createElement('small');loc.textContent=problem.file+':'+problem.line+':'+problem.column;copy.append(msg,loc);row.append(marker,copy);
      row.addEventListener('click',async()=>{if(problem.file&&!problem.file.startsWith('/')&&!/^[A-Za-z]:/.test(problem.file)){if(await openFile(problem.file)){editor.gotoLine(problem.line,problem.column-1,true);editor.focus();}}});
      host.appendChild(row);
    });
  }
  function clearProblems(){problems=[];renderProblems();}
  function toggleProblems(force) {const panel=$('problemsPanel'),show=typeof force==='boolean'?force:panel.classList.contains('hidden');panel.classList.toggle('hidden',!show);}
  async function buildOrRun(mode) {
    if(!root){await openProject();if(!root)return;}
    if(currentPath&&isDirty(currentPath)&&!(await saveCurrent()))return;
    if(!currentPath||!currentPath.toLowerCase().endsWith('.nx')){try{const selected=await api.chooseSource();if(!selected||!(await openFile(selected)))return;}catch(error){appendOutput('Select source failed: '+error.message,'error');return;}}
    if(!currentPath||!currentPath.toLowerCase().endsWith('.nx')){appendOutput('Select a .nx source file before building or running.','error');return;}
    if(!(await ensureCompiler()))return;
    clearProblems();toggleProblems(true);
    try{status(mode==='build'?'Building…':'Running…');appendOutput((mode==='build'?'Build started: ':'Run started: ')+currentPath,'system');if(mode==='build')await api.buildFile(currentPath);else await api.runFile(currentPath);}
    catch(error){appendOutput('Build/run failed: '+error.message,'error');parseDiagnostic('error: '+error.message);status('Failed');}
  }
  function toggleTerminal() {const panel=document.querySelector('.terminal-panel'),collapsed=panel.classList.toggle('collapsed');$('toggleTerminal').textContent=collapsed?'Show':'Hide';}
  async function runTerminalCommand(command) {if(!root){appendOutput('Open a project folder first.','error');return;}appendOutput('> '+command,'command');try{await api.runCommand(command);}catch(error){appendOutput(error.message,'error');}}

  const overlay=$('commandOverlay'),search=$('commandSearch'),list=$('commandList');
  const commands=[
    ['Open project folder','Choose a workspace from your computer','⌂','Ctrl O',openProject],
    ['Create a source file','Add a new file to the current workspace','＋','Ctrl N',newFile],
    ['Save current file','Save the active editor buffer','↧','Ctrl S',saveCurrent],
    ['Save all open files','Save every modified editor tab','▣','',saveAll],
    ['Close active editor tab','Close the current file tab','×','Ctrl W',()=>closeTab(currentPath)],
    ['Select NEXUS compiler','Choose compiler executable or launcher','⌘','',chooseCompiler],
    ['Build current file','Compile the active NEXUS source file','↗','F7',()=>buildOrRun('build')],
    ['Build and run current file','Compile and launch the current source','▶','F5',()=>buildOrRun('run')],
    ['Show Problems panel','Review compiler errors and warnings','!','F8',()=>toggleProblems(true)],
    ['Filter project files','Search filenames in the project explorer','⌕','Ctrl P',()=>$('fileSearch').focus()],
    ['Refresh project explorer','Rescan the active workspace','↻','',refreshTree],
    ['Toggle terminal panel','Show or hide the integrated terminal','›_','',toggleTerminal]
  ];
  let visible=commands.slice(),selected=0;
  function renderCommands() {
    const query=search.value.trim().toLowerCase();visible=commands.filter(c=>(c[0]+' '+c[1]).toLowerCase().includes(query));selected=Math.min(selected,Math.max(visible.length-1,0));list.replaceChildren();
    if(!visible.length){const empty=document.createElement('div');empty.className='command-empty';empty.textContent='No matching commands.';list.appendChild(empty);return;}
    visible.forEach((c,i)=>{const row=document.createElement('button');row.type='button';row.className='command-option'+(i===selected?' selected':'');row.setAttribute('role','option');row.setAttribute('aria-selected',String(i===selected));
      const icon=document.createElement('span');icon.className='command-option-icon';icon.textContent=c[2];const copy=document.createElement('span');copy.className='command-option-copy';
      const title=document.createElement('strong');title.textContent=c[0];const detail=document.createElement('small');detail.textContent=c[1];copy.append(title,detail);row.append(icon,copy);
      if(c[3]){const key=document.createElement('kbd');key.textContent=c[3];row.appendChild(key);}row.addEventListener('click',()=>execute(i));list.appendChild(row);});
  }
  function execute(index=selected){const cmd=visible[index];if(!cmd)return;closePalette();Promise.resolve().then(cmd[4]).catch(e=>appendOutput(cmd[0]+' failed: '+e.message,'error'));}
  function openPalette(){overlay.classList.remove('hidden');overlay.setAttribute('aria-hidden','false');$('openCommands').setAttribute('aria-expanded','true');search.value='';selected=0;renderCommands();search.focus();}
  function closePalette(){overlay.classList.add('hidden');overlay.setAttribute('aria-hidden','true');$('openCommands').setAttribute('aria-expanded','false');$('openCommands').focus();}
  api.onTerminalData(({type,data})=>{if(type==='exit'){appendOutput('['+data+']','system');status('Ready');}else appendOutput(data,type==='stderr'||type==='error'?'error':type);});
  $('openProject').addEventListener('click',openProject);$('welcomeOpen').addEventListener('click',openProject);
  $('newFile').addEventListener('click',newFile);$('welcomeNew').addEventListener('click',newFile);$('saveFile').addEventListener('click',saveCurrent);
  $('selectCompiler').addEventListener('click',chooseCompiler);$('buildFile').addEventListener('click',()=>buildOrRun('build'));$('runFile').addEventListener('click',()=>buildOrRun('run'));
  $('refreshTree').addEventListener('click',refreshTree);$('fileSearch').addEventListener('input',drawTree);
  $('toggleProblems').addEventListener('click',()=>toggleProblems());$('closeProblems').addEventListener('click',()=>toggleProblems(false));
  $('clearTerminal').addEventListener('click',()=>{output.replaceChildren();appendOutput('Output cleared.','system');});
  $('stopTerminal').addEventListener('click',async()=>{try{const stopped=await api.stopCommand();appendOutput(stopped?'Stop requested.':'No command is running.','system');}catch(e){appendOutput('Stop failed: '+e.message,'error');}});
  $('toggleTerminal').addEventListener('click',toggleTerminal);$('openCommands').addEventListener('click',openPalette);$('sidebarOpen').addEventListener('click',openProject);
  $('sidebarCompiler').addEventListener('click',e=>{e.preventDefault();chooseCompiler();});
  search.addEventListener('input',()=>{selected=0;renderCommands();});
  overlay.addEventListener('mousedown',e=>{if(e.target===overlay)closePalette();});
  search.addEventListener('keydown',e=>{if(e.key==='ArrowDown'){e.preventDefault();selected=Math.min(selected+1,visible.length-1);renderCommands();}else if(e.key==='ArrowUp'){e.preventDefault();selected=Math.max(selected-1,0);renderCommands();}else if(e.key==='Enter'){e.preventDefault();execute();}else if(e.key==='Escape'){e.preventDefault();closePalette();}});
  $('terminalForm').addEventListener('submit',async e=>{e.preventDefault();const input=$('terminalInput'),command=input.value.trim();if(!command)return;input.value='';await runTerminalCommand(command);});
  document.addEventListener('keydown',e=>{
    if((e.ctrlKey||e.metaKey)&&e.shiftKey&&e.key.toLowerCase()==='p'){e.preventDefault();openPalette();return;}
    if(!overlay.classList.contains('hidden')){if(e.key==='Escape'){e.preventDefault();closePalette();}return;}
    if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='o'){e.preventDefault();openProject();}
    else if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='s'){e.preventDefault();saveCurrent();}
    else if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='n'){e.preventDefault();newFile();}
    else if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='w'){e.preventDefault();closeTab(currentPath);}
    else if((e.ctrlKey||e.metaKey)&&!e.shiftKey&&e.key.toLowerCase()==='p'){e.preventDefault();$('fileSearch').focus();}
    if(e.key==='F7'){e.preventDefault();buildOrRun('build');}else if(e.key==='F5'){e.preventDefault();buildOrRun('run');}else if(e.key==='F8'){e.preventDefault();toggleProblems();}
  });
  api.platform().then(p=>$('statusPlatform').textContent=({win32:'Windows',darwin:'macOS',linux:'Linux'})[p]||p);
  api.compilerInfo().then(info=>{compilerPath=info.path||null;updateCompilerStatus();}).catch(updateCompilerStatus);
  api.currentProject().then(project=>{if(project.root){root=project.root;treeData=project.tree||[];$('projectName').textContent=projectName();$('projectNameSidebar').textContent=projectName();$('statusProject').textContent=root;drawTree();}}).catch(()=>{});
  renderTabs();renderProblems();
})();