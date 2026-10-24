const { app, BrowserWindow, dialog, ipcMain } = require('electron');
const path = require('node:path');
const fs = require('node:fs/promises');
const { spawn, spawnSync } = require('node:child_process');
const { isInside, assertInsideProject } = require('./path-guard.cjs');

let win;
let projectRoot = null;
let activeProcess = null;
let selectedCompiler = null;

async function assertProjectPath(target) {
  return assertInsideProject(projectRoot, target);
}
function createWindow() {
  win = new BrowserWindow({
    width: 1280, height: 820, minWidth: 820, minHeight: 560,
    backgroundColor: '#1e1e1e',
    title: 'NEXUS Studio',
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    }
  });
  win.loadFile(path.join(__dirname, 'index.html'));
}
function sendOutput(sender, type, data) {
  if (!sender.isDestroyed()) sender.send('terminal:data', { type, data: String(data) });
}
function startProcess(sender, command, args, cwd, options = {}) {
  if (activeProcess) throw new Error('Другая команда ещё выполняется. Сначала останови её.');
  const child = spawn(command, args, {
    cwd,
    env: { ...process.env, ...options.env },
    windowsHide: true,
    stdio: ['ignore', 'pipe', 'pipe']
  });
  activeProcess = child;
  child.stdout.on('data', data => sendOutput(sender, 'stdout', data));
  child.stderr.on('data', data => sendOutput(sender, 'stderr', data));
  child.on('error', error => sendOutput(sender, 'error', error.message));
  child.on('close', code => {
    sendOutput(sender, 'exit', code === null ? 'процесс завершён' : 'код завершения: ' + code);
    if (activeProcess === child) activeProcess = null;
  });
  return true;
}
function settingsPath() {
  return path.join(app.getPath('userData'), 'settings.json');
}
async function loadSettings() {
  try {
    const value = JSON.parse(await fs.readFile(settingsPath(), 'utf8'));
    selectedCompiler = typeof value.compilerPath === 'string' ? value.compilerPath : null;
  } catch { selectedCompiler = null; }
}
async function saveSettings() {
  await fs.mkdir(app.getPath('userData'), { recursive: true });
  await fs.writeFile(settingsPath(), JSON.stringify({ compilerPath: selectedCompiler }, null, 2), 'utf8');
}
async function existsFile(file) {
  if (!file) return false;
  try { return (await fs.stat(file)).isFile(); } catch { return false; }
}
function findOnPath(name) {
  const command = process.platform === 'win32' ? 'where.exe' : 'which';
  const result = spawnSync(command, [name], { encoding: 'utf8', windowsHide: true });
  if (result.status !== 0 || !result.stdout) return null;
  return result.stdout.split(/\r?\n/).map(line => line.trim()).find(Boolean) || null;
}
async function detectCompiler() {
  if (await existsFile(selectedCompiler)) return selectedCompiler;
  selectedCompiler = null;
  const home = app.getPath('home');
  const candidates = process.platform === 'win32' ? [
    process.env.NEXUS_COMPILER,
    path.join(process.env.LOCALAPPDATA || home, 'Programs', 'NEXUS', 'nexus.cmd'),
    path.join(home, 'NEXUS_Windows_Portable', 'nexus.cmd'),
    path.join(home, 'NEXUS', 'nexus.cmd'),
    path.join(home, 'Downloads', 'NEXUS_Windows_Portable', 'nexus.cmd'),
    path.join(process.env.ProgramFiles || 'C:\\Program Files', 'NEXUS', 'nexus.cmd'),
    findOnPath('nexus.cmd'),
    findOnPath('nexus.exe'),
    findOnPath('nexus')
  ] : [
    process.env.NEXUS_COMPILER,
    path.join(home, '.local', 'bin', 'nexus'),
    '/usr/local/bin/nexus',
    '/usr/bin/nexus',
    findOnPath('nexus')
  ];
  for (const candidate of candidates) {
    if (await existsFile(candidate)) {
      selectedCompiler = candidate;
      await saveSettings().catch(() => {});
      return candidate;
    }
  }
  return null;
}
function helperScriptFor(compiler) {
  const name = path.basename(compiler).toLowerCase();
  let root = path.dirname(compiler);
  if (name === 'nexus-core.exe') root = path.dirname(root);
  if (!['nexus.cmd', 'nexus.exe', 'nexus-core.exe', 'nexus.bat'].includes(name)) return null;
  return path.join(root, 'NEXUS-Windows.ps1');
}
function quoteCmd(value) {
  return '"' + String(value).replace(/"/g, '""') + '"';
}
async function compilerInvocation(mode, sourcePath, compiler) {
  const helper = process.platform === 'win32' ? helperScriptFor(compiler) : null;
  if (helper) {
    try {
      await fs.access(helper);
      return {
        command: path.join(process.env.SystemRoot || 'C:\\Windows', 'System32', 'WindowsPowerShell', 'v1.0', 'powershell.exe'),
        args: ['-NoLogo', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', helper, '-Mode', mode === 'build' ? 'Build' : 'Run', '-SourcePath', sourcePath],
        cwd: projectRoot
      };
    } catch { /* standalone CLI: use its documented arguments below */ }
  }
  const outputPath = path.join(path.dirname(sourcePath), path.basename(sourcePath, path.extname(sourcePath)) + (process.platform === 'win32' ? '.exe' : ''));
  const args = mode === 'build'
    ? ['build', sourcePath, '-o', outputPath, '--backend', 'auto']
    : ['run', sourcePath, '--backend', 'auto'];
  if (process.platform === 'win32' && /\.(cmd|bat)$/i.test(compiler)) {
    const commandLine = [compiler, ...args].map(quoteCmd).join(' ');
    return { command: process.env.ComSpec || 'cmd.exe', args: ['/d', '/s', '/c', commandLine], cwd: projectRoot };
  }
  return { command: compiler, args, cwd: projectRoot };
}
async function compileOrRun(event, mode, rel) {
  const source = await assertProjectPath(path.join(projectRoot, rel));
  if (path.extname(source).toLowerCase() !== '.nx') throw new Error('Выбери исходный файл с расширением .nx.');
  if (!(await existsFile(source))) throw new Error('Файл не найден: ' + source);
  const compiler = await detectCompiler();
  if (!compiler) {
    throw new Error('Компилятор NEXUS не найден. Нажми «Компилятор…» и выбери nexus.cmd из установленного NEXUS или nexus-core.exe из папки bin.');
  }
  const invocation = await compilerInvocation(mode, source, compiler);
  sendOutput(event.sender, 'command', (mode === 'build' ? 'BUILD ' : 'RUN ') + source + '\nКомпилятор: ' + compiler);
  return startProcess(event.sender, invocation.command, invocation.args, invocation.cwd);
}
async function readTree(root, depth = 0) {
  if (depth > 5) return [];
  const ignored = new Set(['.git', 'node_modules', 'release', 'dist', 'build', '.cache', '.idea']);
  let entries = [];
  try { entries = await fs.readdir(root, { withFileTypes: true }); } catch { return []; }
  entries.sort((a, b) => Number(b.isDirectory()) - Number(a.isDirectory()) || a.name.localeCompare(b.name));
  const result = [];
  for (const entry of entries) {
    if (entry.name.startsWith('.') && entry.name !== '.gitignore') continue;
    if (ignored.has(entry.name)) continue;
    const full = path.join(root, entry.name);
    const rel = path.relative(projectRoot, full).split(path.sep).join('/');
    if (entry.isDirectory()) {
      result.push({ name: entry.name, path: rel, directory: true, children: await readTree(full, depth + 1) });
    } else if (entry.isFile()) {
      result.push({ name: entry.name, path: rel, directory: false });
    }
  }
  return result;
}

app.whenReady().then(async () => {
  await loadSettings();

  ipcMain.handle('project:open', async () => {
    const result = await dialog.showOpenDialog(win, { properties: ['openDirectory'] });
    if (result.canceled || !result.filePaths[0]) return null;
    projectRoot = result.filePaths[0];
    return { root: projectRoot, tree: await readTree(projectRoot) };
  });
  ipcMain.handle('project:current', async () => ({ root: projectRoot, tree: projectRoot ? await readTree(projectRoot) : [] }));
  ipcMain.handle('project:tree', async () => projectRoot ? readTree(projectRoot) : []);
  ipcMain.handle('file:chooseSource', async () => {
    if (!projectRoot) throw new Error('Сначала открой папку проекта.');
    const result = await dialog.showOpenDialog(win, {
      defaultPath: projectRoot,
      properties: ['openFile'],
      filters: [{ name: 'NEXUS source files', extensions: ['nx'] }, { name: 'All files', extensions: ['*'] }]
    });
    if (result.canceled || !result.filePaths[0]) return null;
    const chosen = path.resolve(result.filePaths[0]);
    await assertProjectPath(chosen);
    if (!isInside(projectRoot, chosen)) throw new Error('Исходный файл должен находиться внутри открытого проекта.');
    return path.relative(projectRoot, chosen).split(path.sep).join('/');
  });
  ipcMain.handle('file:read', async (_event, rel) => {
    const target = await assertProjectPath(path.join(projectRoot, rel));
    const stat = await fs.stat(target);
    if (!stat.isFile() || stat.size > 2 * 1024 * 1024) throw new Error('Файл нельзя открыть или его размер превышает 2 МБ.');
    return fs.readFile(target, 'utf8');
  });
  ipcMain.handle('file:save', async (_event, rel, content) => {
    const target = await assertProjectPath(path.join(projectRoot, rel));
    await fs.mkdir(path.dirname(target), { recursive: true });
    await fs.writeFile(target, String(content), 'utf8');
    return true;
  });
  ipcMain.handle('file:new', async (_event, rel) => {
    const target = await assertProjectPath(path.join(projectRoot, rel));
    await fs.mkdir(path.dirname(target), { recursive: true });
    await fs.writeFile(target, '', { flag: 'wx' });
    return true;
  });
  ipcMain.handle('terminal:run', async (event, command) => {
    if (!projectRoot) throw new Error('Сначала открой папку проекта.');
    const shell = process.platform === 'win32' ? (process.env.ComSpec || 'cmd.exe') : '/bin/sh';
    const args = process.platform === 'win32' ? ['/d', '/s', '/c', String(command)] : ['-lc', String(command)];
    return startProcess(event.sender, shell, args, projectRoot);
  });
  ipcMain.handle('terminal:stop', async () => {
    if (!activeProcess) return false;
    const pid = activeProcess.pid;
    if (process.platform === 'win32' && pid) {
      spawn('taskkill.exe', ['/PID', String(pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
    } else {
      activeProcess.kill('SIGTERM');
    }
    activeProcess = null;
    return true;
  });
  ipcMain.handle('compiler:info', async () => {
    const compiler = await detectCompiler();
    return { path: compiler, available: !!compiler };
  });
  ipcMain.handle('compiler:choose', async () => {
    const filters = process.platform === 'win32'
      ? [{ name: 'NEXUS launcher or compiler', extensions: ['cmd', 'exe', 'bat'] }, { name: 'All files', extensions: ['*'] }]
      : [{ name: 'NEXUS compiler', extensions: ['*'] }];
    const result = await dialog.showOpenDialog(win, { properties: ['openFile'], filters });
    if (result.canceled || !result.filePaths[0]) return null;
    selectedCompiler = result.filePaths[0];
    if (!(await existsFile(selectedCompiler))) throw new Error('Выбранный файл не найден.');
    await saveSettings();
    return { path: selectedCompiler, available: true };
  });
  ipcMain.handle('compiler:build', async (event, rel) => compileOrRun(event, 'build', rel));
  ipcMain.handle('compiler:run', async (event, rel) => compileOrRun(event, 'run', rel));
  ipcMain.handle('app:platform', () => process.platform);

  createWindow();
  app.on('activate', () => { if (BrowserWindow.getAllWindows().length === 0) createWindow(); });
});
app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
