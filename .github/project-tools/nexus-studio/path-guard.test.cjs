const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { assertInsideProject } = require('./path-guard.cjs');

async function makeSandbox(t) {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'nexus-studio-path-'));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  return root;
}

test('accepts an existing file inside the opened project', async t => {
  const sandbox = await makeSandbox(t);
  const project = path.join(sandbox, 'project');
  const source = path.join(project, 'src', 'main.nx');
  await fs.mkdir(path.dirname(source), { recursive: true });
  await fs.writeFile(source, 'fn main() {}', 'utf8');

  assert.equal(await assertInsideProject(project, source), source);
});

test('accepts a not-yet-created file whose parents are inside the project', async t => {
  const sandbox = await makeSandbox(t);
  const project = path.join(sandbox, 'project');
  const target = path.join(project, 'new', 'nested', 'main.nx');
  await fs.mkdir(project, { recursive: true });

  assert.equal(await assertInsideProject(project, target), target);
});

test('rejects lexical traversal outside the project', async t => {
  const sandbox = await makeSandbox(t);
  const project = path.join(sandbox, 'project');
  await fs.mkdir(project, { recursive: true });

  await assert.rejects(
    assertInsideProject(project, path.join(project, '..', 'outside', 'secret.nx')),
    /за пределами проекта/
  );
});

test('rejects symlinks that lead outside the project', { skip: process.platform === 'win32' ? 'Symlink creation permissions differ on Windows runners.' : false }, async t => {
  const sandbox = await makeSandbox(t);
  const project = path.join(sandbox, 'project');
  const outside = path.join(sandbox, 'outside');
  await fs.mkdir(project, { recursive: true });
  await fs.mkdir(outside, { recursive: true });
  await fs.writeFile(path.join(outside, 'secret.nx'), 'not project content', 'utf8');
  await fs.symlink(outside, path.join(project, 'linked'), 'dir');

  await assert.rejects(
    assertInsideProject(project, path.join(project, 'linked', 'secret.nx')),
    /за пределами проекта/
  );
  await assert.rejects(
    assertInsideProject(project, path.join(project, 'linked', 'new.nx')),
    /за пределами проекта/
  );
});
