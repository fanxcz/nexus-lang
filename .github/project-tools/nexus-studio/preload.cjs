const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('nexusStudio', {
  openProject: () => ipcRenderer.invoke('project:open'),
  currentProject: () => ipcRenderer.invoke('project:current'),
  refreshTree: () => ipcRenderer.invoke('project:tree'),
  chooseSource: () => ipcRenderer.invoke('file:chooseSource'),
  readFile: filePath => ipcRenderer.invoke('file:read', filePath),
  saveFile: (filePath, content) => ipcRenderer.invoke('file:save', filePath, content),
  newFile: filePath => ipcRenderer.invoke('file:new', filePath),
  runCommand: command => ipcRenderer.invoke('terminal:run', command),
  stopCommand: () => ipcRenderer.invoke('terminal:stop'),
  compilerInfo: () => ipcRenderer.invoke('compiler:info'),
  chooseCompiler: () => ipcRenderer.invoke('compiler:choose'),
  buildFile: filePath => ipcRenderer.invoke('compiler:build', filePath),
  runFile: filePath => ipcRenderer.invoke('compiler:run', filePath),
  platform: () => ipcRenderer.invoke('app:platform'),
  onTerminalData: callback => ipcRenderer.on('terminal:data', (_event, data) => callback(data)),
  startLanguageServer: () => ipcRenderer.invoke('lsp:start'),
  openLanguageDocument: (filePath, content, version) =>
    ipcRenderer.invoke('lsp:open', filePath, content, version),
  changeLanguageDocument: (filePath, content, version) =>
    ipcRenderer.invoke('lsp:change', filePath, content, version),
  saveLanguageDocument: filePath => ipcRenderer.invoke('lsp:save', filePath),
  closeLanguageDocument: filePath => ipcRenderer.invoke('lsp:close', filePath),
  findDefinition: (filePath, position) =>
    ipcRenderer.invoke('lsp:definition', filePath, position),
  getCompletions: (filePath, position) =>
    ipcRenderer.invoke('lsp:completion', filePath, position),
  getHover: (filePath, position) =>
    ipcRenderer.invoke('lsp:hover', filePath, position),
  getSignatureHelp: (filePath, position) =>
    ipcRenderer.invoke('lsp:signatureHelp', filePath, position),
  onDiagnostics: callback =>
    ipcRenderer.on('lsp:diagnostics', (_event, diagnostics) => callback(diagnostics)),
  onLanguageServerExit: callback =>
    ipcRenderer.on('lsp:exit', (_event, status) => callback(status))
});
