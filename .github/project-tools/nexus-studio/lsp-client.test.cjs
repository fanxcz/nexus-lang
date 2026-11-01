const test = require('node:test');
const assert = require('node:assert/strict');
const { EventEmitter } = require('node:events');
const { PassThrough } = require('node:stream');
const {
  JsonRpcStreamDecoder,
  NexusLspClient,
  encodeMessage
} = require('./lsp-client.cjs');

function readFrames(buffer, decoder) {
  return decoder.push(buffer);
}

class FakeChild extends EventEmitter {
  constructor() {
    super();
    this.stdin = new PassThrough();
    this.stdout = new PassThrough();
    this.stderr = new PassThrough();
    this.exitCode = null;
    this.signalCode = null;
    this.received = [];
    this.inputDecoder = new JsonRpcStreamDecoder();
    this.stdin.on('data', chunk => {
      for (const message of this.inputDecoder.push(chunk)) {
        this.received.push(message);
        if (message.method === 'initialize') {
          // Server-originated requests use an independent ID space and may
          // reuse the client's outstanding initialize ID.
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0',
            id: message.id,
            method: 'client/registerCapability',
            params: { registrations: [] }
          }));
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0',
            id: message.id,
            result: { capabilities: { textDocumentSync: 1, definitionProvider: true } }
          }));
        } else if (message.method === 'shutdown') {
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0', id: message.id, result: null
          }));
        } else if (message.method === 'textDocument/completion') {
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0',
            id: message.id,
            result: [{ label: 'print', kind: 14, detail: 'NEXUS builtin' }]
          }));
        } else if (message.method === 'textDocument/hover') {
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0',
            id: message.id,
            result: { contents: { kind: 'markdown', value: '**print**' } }
          }));
        }
      }
    });
    this.stdin.on('end', () => {
      if (this.exitCode === null && this.signalCode === null) {
        this.exitCode = 0;
        this.emit('close', 0, null);
      }
    });
  }

  kill(signal = 'SIGTERM') {
    if (this.exitCode !== null || this.signalCode !== null) return false;
    this.signalCode = signal;
    this.emit('close', null, signal);
    return true;
  }
}

test('encodeMessage uses UTF-8 byte length for JSON bodies', () => {
  const frame = encodeMessage({ jsonrpc: '2.0', method: 'test', params: { text: 'NEXUS 🚀' } });
  const separator = frame.indexOf(Buffer.from('\r\n\r\n'));
  assert.notEqual(separator, -1);
  const header = frame.subarray(0, separator).toString('ascii');
  const body = frame.subarray(separator + 4);
  const match = /^Content-Length: (\d+)$/i.exec(header);
  assert.ok(match);
  assert.equal(Number(match[1]), Buffer.byteLength(body));
  assert.equal(JSON.parse(body.toString('utf8')).params.text, 'NEXUS 🚀');
});

test('decoder handles fragmented frames and multiple messages in one chunk', () => {
  const first = encodeMessage({ jsonrpc: '2.0', id: 1, result: 'Привет 🌍' });
  const second = encodeMessage({ jsonrpc: '2.0', method: 'workspace/test', params: { ok: true } });
  const decoder = new JsonRpcStreamDecoder();
  const joined = Buffer.concat([first, second]);
  assert.deepEqual(decoder.push(joined.subarray(0, 7)), []);
  assert.deepEqual(decoder.push(joined.subarray(7, first.length - 2)), []);
  const messages = decoder.push(joined.subarray(first.length - 2));
  assert.equal(messages.length, 2);
  assert.equal(messages[0].result, 'Привет 🌍');
  assert.equal(messages[1].method, 'workspace/test');
});

test('decoder accepts case-insensitive Content-Length with additional headers', () => {
  const body = Buffer.from('{"jsonrpc":"2.0","id":3,"result":true}', 'utf8');
  const header = Buffer.from(
    'content-type: application/vscode-jsonrpc; charset=utf-8\r\nContent-Length: ' +
    body.length + '\r\n\r\n',
    'ascii'
  );
  const decoder = new JsonRpcStreamDecoder();
  assert.deepEqual(decoder.push(Buffer.concat([header, body])), [
    { jsonrpc: '2.0', id: 3, result: true }
  ]);
});

test('decoder rejects missing, duplicate, malformed and oversized Content-Length', () => {
  assert.throws(() =>
    new JsonRpcStreamDecoder(32).push(Buffer.from('X-Test: yes\r\n\r\n{}')),
    /exactly one Content-Length/
  );
  assert.throws(() =>
    new JsonRpcStreamDecoder(32).push(
      Buffer.from('Content-Length: 2\r\ncontent-length: 2\r\n\r\n{}')
    ),
    /exactly one Content-Length/
  );
  assert.throws(() =>
    new JsonRpcStreamDecoder(32).push(Buffer.from('Content-Length: nope\r\n\r\n{}')),
    /invalid/
  );
  assert.throws(() =>
    new JsonRpcStreamDecoder(32).push(Buffer.from('Content-Length: 33\r\n\r\n')),
    /permitted range/
  );
  assert.throws(() =>
    new JsonRpcStreamDecoder(32).push(Buffer.from('x'.repeat(16 * 1024 + 1))),
    /header exceeds/
  );
});

test('decoder rejects non-object and invalid JSON payloads', () => {
  const arrayBody = Buffer.from('[]');
  const arrayFrame = Buffer.concat([
    Buffer.from('Content-Length: 2\r\n\r\n'), arrayBody
  ]);
  assert.throws(() => new JsonRpcStreamDecoder().push(arrayFrame), /must be an object/);

  const invalidBody = Buffer.from('{broken');
  const invalidFrame = Buffer.concat([
    Buffer.from('Content-Length: ' + invalidBody.length + '\r\n\r\n'), invalidBody
  ]);
  assert.throws(() => new JsonRpcStreamDecoder().push(invalidFrame), /not valid JSON/);

  const invalidUtf8Body = Buffer.from([0xff]);
  const invalidUtf8Frame = Buffer.concat([
    Buffer.from('Content-Length: 1\r\n\r\n'), invalidUtf8Body
  ]);
  assert.throws(() => new JsonRpcStreamDecoder().push(invalidUtf8Frame), /not valid UTF-8/);
});

test('language client initializes, acknowledges server requests and forwards diagnostics', async () => {
  const child = new FakeChild();
  let spawnCall;
  const client = new NexusLspClient({
    platform: 'linux',
    env: { PATH: '/bin' },
    spawnProcess: (...args) => {
      spawnCall = args;
      return child;
    }
  });
  const diagnostics = [];
  client.on('diagnostics', value => diagnostics.push(value));

  const started = await client.start('/opt/nexus/bin/nexus', '/tmp/NEXUS project');
  assert.equal(started.ready, true);
  assert.equal(started.reused, false);
  assert.equal(client.ready, true);
  assert.equal(spawnCall[0], '/opt/nexus/bin/nexus');
  assert.deepEqual(spawnCall[1], ['lsp']);
  assert.ok(spawnCall[2].cwd.endsWith('NEXUS project'));

  const initialize = child.received.find(message => message.method === 'initialize');
  assert.ok(initialize);
  assert.ok(initialize.params.rootPath.endsWith('NEXUS project'));
  assert.ok(initialize.params.rootUri.endsWith('/NEXUS%20project'));
  assert.equal(initialize.params.workspaceFolders[0].name, 'NEXUS project');
  assert.ok(child.received.some(message => message.method === 'initialized'));
  assert.ok(child.received.some(message =>
    message.id === initialize.id && message.result === null
  ));

  child.stdout.write(encodeMessage({
    jsonrpc: '2.0',
    id: 'register-watch',
    method: 'client/registerCapability',
    params: { registrations: [] }
  }));
  await new Promise(resolve => setImmediate(resolve));
  assert.ok(child.received.some(message => message.id === 'register-watch' && message.result === null));

  child.stdout.write(encodeMessage({
    jsonrpc: '2.0',
    id: 'unsupported-request',
    method: 'workspace/applyEdit',
    params: {}
  }));
  await new Promise(resolve => setImmediate(resolve));
  assert.ok(child.received.some(message =>
    message.id === 'unsupported-request' && message.error?.code === -32601
  ));

  child.stdout.write(encodeMessage({
    jsonrpc: '2.0',
    method: 'textDocument/publishDiagnostics',
    params: {
      uri: 'file:///tmp/NEXUS%20project/main.nx',
      diagnostics: [{ message: 'expected expression', severity: 1 }]
    }
  }));
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(diagnostics.length, 1);
  assert.equal(diagnostics[0].diagnostics[0].message, 'expected expression');

  const uri = 'file:///tmp/NEXUS%20project/main.nx';
  assert.equal(client.openDocument(uri, 'fn main() {}', 1), true);
  assert.equal(client.changeDocument(uri, 'fn main() { print(1) }', 2), true);
  assert.equal(client.saveDocument(uri), true);
  assert.equal(client.closeDocument(uri), true);
  assert.deepEqual(
    child.received.filter(message => /^textDocument\/did/.test(message.method || '')).map(x => x.method),
    ['textDocument/didOpen', 'textDocument/didChange', 'textDocument/didSave', 'textDocument/didClose']
  );

  const completions = await client.request('textDocument/completion', {
    textDocument: { uri }, position: { line: 0, character: 4 }
  });
  assert.equal(completions[0].label, 'print');
  const hover = await client.request('textDocument/hover', {
    textDocument: { uri }, position: { line: 0, character: 1 }
  });
  assert.equal(hover.contents.value, '**print**');

  await client.stop();
  assert.equal(client.ready, false);
});

test('language client rejects new work before initialization and reuses the same workspace', async () => {
  const child = new FakeChild();
  const client = new NexusLspClient({
    platform: 'linux',
    spawnProcess: () => child
  });
  await assert.rejects(client.request('textDocument/hover'), /not ready/);
  await client.start('/usr/bin/nexus', '/tmp/project');
  const reused = await client.start('/usr/bin/nexus', '/tmp/project');
  assert.deepEqual(reused, { ready: true, reused: true });
  await client.stop();
});

test('Windows command launch quotes a compiler path with spaces', async () => {
  const child = new FakeChild();
  let spawnCall;
  const client = new NexusLspClient({
    platform: 'win32',
    env: { ComSpec: 'C:\\Windows\\System32\\cmd.exe' },
    spawnProcess: (...args) => { spawnCall = args; return child; }
  });
  await client.start('C:\\Program Files\\NEXUS\\nexus.cmd', 'C:\\work\\project');
  assert.equal(spawnCall[0], 'C:\\Windows\\System32\\cmd.exe');
  assert.deepEqual(spawnCall[1], [
    '/d', '/s', '/c',
    '"C:\\Program Files\\NEXUS\\nexus.cmd" "lsp"'
  ]);
  await client.stop();
});

test('stopping the child rejects outstanding requests', async () => {
  const child = new FakeChild();
  const client = new NexusLspClient({
    platform: 'linux',
    spawnProcess: () => child
  });
  await client.start('/usr/bin/nexus', '/tmp/project');
  const pending = client.request('workspace/symbol', { query: 'x' });
  await new Promise(resolve => setImmediate(resolve));
  child.kill('SIGTERM');
  await assert.rejects(pending, /exited/);
  assert.equal(client.ready, false);
  assert.equal(client.child, null);
});

test('stop observes a child that closes immediately after stdin is ended', async () => {
  const child = new FakeChild();
  child.stdin.end = () => {
    child.exitCode = 0;
    child.emit('close', 0, null);
  };
  const client = new NexusLspClient({ spawnProcess: () => child });
  client.child = child;
  await client.stop();
  assert.equal(client.child, null);
});

test('decoder constructor rejects invalid limits', () => {
  assert.throws(() => new JsonRpcStreamDecoder(0), /positive safe integer/);
  assert.throws(() => new JsonRpcStreamDecoder(Number.MAX_SAFE_INTEGER + 1), /positive safe integer/);
});
