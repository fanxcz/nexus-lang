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
  onTerminalData: callback => ipcRenderer.on('terminal:data', (_event, data) => callback(data))
});
