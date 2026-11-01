const { EventEmitter } = require('node:events');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { spawn } = require('node:child_process');

const MAX_HEADER_BYTES = 16 * 1024;
const DEFAULT_MAX_MESSAGE_BYTES = 16 * 1024 * 1024;

function encodeMessage(message) {
  const body = Buffer.from(JSON.stringify(message), 'utf8');
  const header = Buffer.from('Content-Length: ' + body.length + '\r\n\r\n', 'ascii');
  return Buffer.concat([header, body]);
}

class JsonRpcStreamDecoder {
  constructor(maxMessageBytes = DEFAULT_MAX_MESSAGE_BYTES) {
    if (!Number.isSafeInteger(maxMessageBytes) || maxMessageBytes < 1) {
      throw new TypeError('maxMessageBytes must be a positive safe integer');
    }
    this.maxMessageBytes = maxMessageBytes;
    this.buffer = Buffer.alloc(0);
  }

  push(chunk) {
    if (!Buffer.isBuffer(chunk)) chunk = Buffer.from(chunk);
    if (chunk.length) this.buffer = Buffer.concat([this.buffer, chunk]);
    const messages = [];

    while (this.buffer.length) {
      const headerEnd = this.buffer.indexOf('\r\n\r\n');
      if (headerEnd < 0) {
        if (this.buffer.length > MAX_HEADER_BYTES) {
          throw new Error('JSON-RPC header exceeds the size limit');
        }
        break;
      }
      if (headerEnd > MAX_HEADER_BYTES) {
        throw new Error('JSON-RPC header exceeds the size limit');
      }

      const header = this.buffer.subarray(0, headerEnd).toString('ascii');
      const contentLengthHeaders = header.split('\r\n').filter(line =>
        /^content-length\s*:/i.test(line)
      );
      if (contentLengthHeaders.length !== 1) {
        throw new Error('JSON-RPC message must contain exactly one Content-Length header');
      }

      const match = /^content-length\s*:\s*(\d+)\s*$/i.exec(contentLengthHeaders[0]);
      if (!match) throw new Error('JSON-RPC Content-Length is invalid');
      const contentLength = Number(match[1]);
      if (!Number.isSafeInteger(contentLength) || contentLength < 1 ||
          contentLength > this.maxMessageBytes) {
        throw new Error('JSON-RPC message length is outside the permitted range');
      }

      const bodyStart = headerEnd + 4;
      const frameEnd = bodyStart + contentLength;
      if (!Number.isSafeInteger(frameEnd)) {
        throw new Error('JSON-RPC frame length overflow');
      }
      if (this.buffer.length < frameEnd) break;

      const body = this.buffer.subarray(bodyStart, frameEnd);
      let text;
      try {
        text = new TextDecoder('utf-8', { fatal: true }).decode(body);
      } catch (error) {
        throw new Error('JSON-RPC body is not valid UTF-8: ' + error.message);
      }
      let message;
      try {
        message = JSON.parse(text);
      } catch (error) {
        throw new Error('JSON-RPC body is not valid JSON: ' + error.message);
      }
      if (!message || typeof message !== 'object' || Array.isArray(message)) {
        throw new Error('JSON-RPC body must be an object');
      }
      messages.push(message);
      this.buffer = this.buffer.subarray(frameEnd);
    }
    return messages;
  }
}

function compilerInvocation(compiler, platform, env) {
  if (platform === 'win32' && /\.(cmd|bat)$/i.test(compiler)) {
    const quote = value => '"' + String(value).replace(/"/g, '""') + '"';
    return {
      command: env.ComSpec || 'cmd.exe',
      args: ['/d', '/s', '/c', [compiler, 'lsp'].map(quote).join(' ')]
    };
  }
  return { command: compiler, args: ['lsp'] };
}

class NexusLspClient extends EventEmitter {
  constructor(options = {}) {
    super();
    this.spawnProcess = options.spawnProcess || spawn;
    this.platform = options.platform || process.platform;
    this.env = options.env || process.env;
    this.child = null;
    this.compiler = null;
    this.root = null;
    this.ready = false;
    this.decoder = null;
    this.nextRequestId = 1;
    this.pending = new Map();
  }

  async start(compiler, projectRoot) {
    if (typeof compiler !== 'string' || !compiler.trim()) {
      throw new Error('NEXUS compiler path is required to start language services');
    }
    if (typeof projectRoot !== 'string' || !projectRoot.trim()) {
      throw new Error('A project folder is required to start language services');
    }

    const root = path.resolve(projectRoot);
    if (this.child && this.compiler === compiler && this.root === root && this.ready) {
      return { ready: true, reused: true };
    }
    await this.stop();

    const invocation = compilerInvocation(compiler, this.platform, this.env);
    const child = this.spawnProcess(invocation.command, invocation.args, {
      cwd: root,
      env: { ...this.env },
      windowsHide: true,
      stdio: ['pipe', 'pipe', 'pipe']
    });

    this.child = child;
    this.compiler = compiler;
    this.root = root;
    this.ready = false;
    this.decoder = new JsonRpcStreamDecoder();

    child.stdout.on('data', chunk => {
      try {
        for (const message of this.decoder.push(chunk)) this._dispatch(message);
      } catch (error) {
        this.emit('error', error);
        this._failPending(error);
        child.kill();
      }
    });
    child.stderr.on('data', chunk => this.emit('stderr', chunk.toString('utf8')));
    child.on('error', error => {
      this.emit('error', error);
      this._failPending(error);
    });
    child.on('close', (code, signal) => {
      if (this.child === child) {
        this.child = null;
        this.ready = false;
      }
      const reason = new Error('NEXUS language server exited' +
        (code === null ? '' : ' with code ' + code) +
        (signal ? ' (' + signal + ')' : ''));
      this._failPending(reason);
      this.emit('exit', { code, signal });
    });

    const rootUri = pathToFileURL(root).href;
    try {
      const result = await this._request('initialize', {
        processId: process.pid,
        rootUri,
        rootPath: root,
        workspaceFolders: [{ uri: rootUri, name: path.basename(root) || root }],
        capabilities: {
          workspace: { didChangeWatchedFiles: { dynamicRegistration: false } },
          textDocument: {
            publishDiagnostics: { relatedInformation: true },
            synchronization: { didSave: true, dynamicRegistration: false }
          }
        },
        clientInfo: { name: 'NEXUS Studio', version: '0.2.0' }
      });
      this.notify('initialized', {});
      this.ready = true;
      this.emit('ready', result && result.capabilities ? result.capabilities : {});
      return { ready: true, reused: false, capabilities: result?.capabilities || {} };
    } catch (error) {
      await this.stop();
      throw error;
    }
  }

  _send(message) {
    if (!this.child || !this.child.stdin || this.child.stdin.destroyed ||
        !this.child.stdin.writable) {
      throw new Error('NEXUS language server is not running');
    }
    this.child.stdin.write(encodeMessage(message));
  }

  _request(method, params, timeoutMs = 15000) {
    const id = this.nextRequestId++;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error('NEXUS language server timed out on ' + method));
      }, timeoutMs);
      this.pending.set(id, { resolve, reject, timer });
      try {
        this._send({ jsonrpc: '2.0', id, method, params });
      } catch (error) {
        clearTimeout(timer);
        this.pending.delete(id);
        reject(error);
      }
    });
  }

  request(method, params = {}) {
    if (!this.ready) return Promise.reject(new Error('NEXUS language server is not ready'));
    return this._request(method, params);
  }

  notify(method, params = {}) {
    if (!this.child) return false;
    this._send({ jsonrpc: '2.0', method, params });
    return true;
  }

  _dispatch(message) {
    // Requests and notifications from the server have their own ID space;
    // a server request may legally reuse an ID of a pending client request.
    if (typeof message.method === 'string') {
      if (message.method === 'client/registerCapability' ||
          message.method === 'client/unregisterCapability') {
        if (Object.prototype.hasOwnProperty.call(message, 'id')) {
          this._send({ jsonrpc: '2.0', id: message.id, result: null });
        }
        return;
      }
      if (message.method === 'textDocument/publishDiagnostics') {
        this.emit('diagnostics', message.params || {});
        return;
      }
      if (Object.prototype.hasOwnProperty.call(message, 'id')) {
        this._send({
          jsonrpc: '2.0',
          id: message.id,
          error: { code: -32601, message: 'Method not found: ' + message.method }
        });
      } else {
        this.emit('notification', message);
      }
      return;
    }

    if (Object.prototype.hasOwnProperty.call(message, 'id') && this.pending.has(message.id)) {
      const pending = this.pending.get(message.id);
      this.pending.delete(message.id);
      clearTimeout(pending.timer);
      if (message.error) {
        pending.reject(new Error(message.error.message || 'NEXUS language server request failed'));
      } else {
        pending.resolve(message.result);
      }
    }
  }

  _failPending(error) {
    for (const pending of this.pending.values()) {
      clearTimeout(pending.timer);
      pending.reject(error);
    }
    this.pending.clear();
  }

  openDocument(uri, text, version = 1) {
    if (!this.ready) return false;
    return this.notify('textDocument/didOpen', {
      textDocument: { uri, languageId: 'nexus', version, text: String(text) }
    });
  }

  changeDocument(uri, text, version) {
    if (!this.ready) return false;
    return this.notify('textDocument/didChange', {
      textDocument: { uri, version },
      contentChanges: [{ text: String(text) }]
    });
  }

  saveDocument(uri) {
    if (!this.ready) return false;
    return this.notify('textDocument/didSave', { textDocument: { uri } });
  }

  closeDocument(uri) {
    if (!this.ready) return false;
    return this.notify('textDocument/didClose', { textDocument: { uri } });
  }

  async stop() {
    const child = this.child;
    if (!child) {
      this.ready = false;
      this._failPending(new Error('NEXUS language server stopped'));
      return;
    }

    if (this.ready) {
      try {
        await this._request('shutdown', {}, 2000);
        this.notify('exit', {});
      } catch { /* The child may already be exiting. */ }
    }
    this.ready = false;
    this._failPending(new Error('NEXUS language server stopped'));

    // Attach the close listener before ending stdin so a fast child exit cannot
    // race past the waiter and leave a stale process reference behind.
    let waitForClose = null;
    if (child.exitCode === null && child.signalCode === null) {
      waitForClose = new Promise(resolve => {
        let finished = false;
        let killTimer;
        let graceTimer;
        const finish = () => {
          if (finished) return;
          finished = true;
          clearTimeout(killTimer);
          clearTimeout(graceTimer);
          child.off('close', finish);
          resolve();
        };
        child.once('close', finish);
        killTimer = setTimeout(() => {
          if (child.exitCode !== null || child.signalCode !== null) return finish();
          child.kill();
          graceTimer = setTimeout(finish, 750);
          graceTimer.unref?.();
        }, 1200);
        killTimer.unref?.();
      });
    }

    if (child.stdin && child.stdin.writable) child.stdin.end();
    if (waitForClose) await waitForClose;
    if (this.child === child) this.child = null;
  }
}

module.exports = { JsonRpcStreamDecoder, NexusLspClient, encodeMessage };
