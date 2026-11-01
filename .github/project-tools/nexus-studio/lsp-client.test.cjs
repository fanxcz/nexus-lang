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
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0',
            id: message.id,
            result: { capabilities: { textDocumentSync: 1, definitionProvider: true } }
          }));
        } else if (message.method === 'shutdown') {
          this.stdout.write(encodeMessage({
            jsonrpc: '2.0', id: message.id, result: null
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
  const decoder = new JsonRpcStreamDecoder(32);
  assert.throws(() => decoder.push(Buffer.from('X-Test: yes\r\n\r\n{}')), /exactly one Content-Length/);
  assert.throws(() => decoder.push(Buffer.from('Content-Length: 2\r\ncontent-length: 2\r\n\r\n{}')),
    /exactly one Content-Length/);
  assert.throws(() => decoder.push(Buffer.from('Content-Length: nope\r\n\r\n{}')), /invalid/);
  assert.throws(() => decoder.push(Buffer.from('Content-Length: 33\r\n\r\n')), /permitted range/);
  assert.throws(() => decoder.push(Buffer.from('x'.repeat(16 * 1024 + 1))), /header exceeds/);
});

test('decoder rejects non-object and invalid JSON payloads', () => {
  const decoder = new JsonRpcStreamDecoder();
  const arrayBody = Buffer.from('[]');
  const arrayFrame = Buffer.concat([
    Buffer.from('Content-Length: 2\r\n\r\n'), arrayBody
  ]);
  assert.throws(() => decoder.push(arrayFrame), /must be an object/);

  const invalidBody = Buffer.from('{broken');
  const invalidFrame = Buffer.concat([
    Buffer.from('Content-Length: ' + invalidBody.length + '\r\n\r\n'), invalidBody
  ]);
  assert.throws(() => decoder.push(invalidFrame), /not valid JSON/);
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
  assert.equal(spawnCall[2].cwd, '/tmp/NEXUS project');

  const initialize = child.received.find(message => message.method === 'initialize');
  assert.ok(initialize);
  assert.equal(initialize.params.rootPath, '/tmp/NEXUS project');
  assert.equal(initialize.params.workspaceFolders[0].name, 'NEXUS project');
  assert.ok(child.received.some(message => message.method === 'initialized'));

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
  class SilentChild extends FakeChild {
    constructor() {
      super();
      this.stdin.removeAllListeners('data');
    }
  }
  const child = new SilentChild();
  const client = new NexusLspClient({
    platform: 'linux',
    spawnProcess: () => child
  });
  client.child = child;
  client.ready = true;
  const pending = client.request('workspace/symbol', { query: 'x' });
  await new Promise(resolve => setImmediate(resolve));
  child.kill('SIGTERM');
  await assert.rejects(pending, /exited/);
});

test('decoder constructor rejects invalid limits', () => {
  assert.throws(() => new JsonRpcStreamDecoder(0), /positive safe integer/);
  assert.throws(() => new JsonRpcStreamDecoder(Number.MAX_SAFE_INTEGER + 1), /positive safe integer/);
});
