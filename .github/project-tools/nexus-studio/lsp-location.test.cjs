const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { toProjectLocation } = require('./lsp-location.cjs');

const validRange = {
  start: { line: 2, character: 4 },
  end: { line: 2, character: 11 }
};

async function createFixture(t) {
  const tempRoot = await fs.mkdtemp(path.join(os.tmpdir(), 'nexus-lsp-location-'));
  const projectRoot = path.join(tempRoot, 'project');
  const sourceRoot = path.join(projectRoot, 'src');
  const outsideRoot = path.join(tempRoot, 'outside');
  await fs.mkdir(sourceRoot, { recursive: true });
  await fs.mkdir(outsideRoot, { recursive: true });
  const sourceFile = path.join(sourceRoot, 'main.nx');
  const outsideFile = path.join(outsideRoot, 'outside.nx');
  await fs.writeFile(sourceFile, 'fn main() {}\n');
  await fs.writeFile(outsideFile, 'fn outside() {}\n');
  t.after(() => fs.rm(tempRoot, { recursive: true, force: true }));
  return { projectRoot, sourceFile, outsideFile, sourceRoot };
}

test('maps an in-project file URI to a project-relative NEXUS path', async t => {
  const fixture = await createFixture(t);
  const location = await toProjectLocation({
    uri: pathToFileURL(fixture.sourceFile).href,
    range: validRange
  }, fixture.projectRoot);
  assert.deepEqual(location, { path: 'src/main.nx', range: validRange });
});

test('supports LocationLink targetUri and targetSelectionRange', async t => {
  const fixture = await createFixture(t);
  const location = await toProjectLocation({
    targetUri: pathToFileURL(fixture.sourceFile).href,
    targetRange: { start: { line: 0, character: 0 }, end: { line: 0, character: 12 } },
    targetSelectionRange: validRange
  }, fixture.projectRoot);
  assert.deepEqual(location, { path: 'src/main.nx', range: validRange });
});

test('rejects non-file, malformed, missing and non-NEXUS targets', async t => {
  const fixture = await createFixture(t);
  assert.equal(await toProjectLocation({ uri: 'https://example.test/main.nx', range: validRange },
    fixture.projectRoot), null);
  assert.equal(await toProjectLocation({ uri: 'file:///bad%ZZ', range: validRange },
    fixture.projectRoot), null);
  assert.equal(await toProjectLocation({
    uri: pathToFileURL(path.join(fixture.sourceRoot, 'missing.nx')).href,
    range: validRange
  }, fixture.projectRoot), null);
  assert.equal(await toProjectLocation({
    uri: pathToFileURL(path.join(fixture.sourceRoot, 'readme.md')).href,
    range: validRange
  }, fixture.projectRoot), null);
  assert.equal(await toProjectLocation({
    uri: pathToFileURL(fixture.sourceFile).href,
    range: { start: { line: -1, character: 0 }, end: { line: 0, character: 1 } }
  }, fixture.projectRoot), null);
});

test('rejects definition targets outside the opened project', async t => {
  const fixture = await createFixture(t);
  const location = await toProjectLocation({
    uri: pathToFileURL(fixture.outsideFile).href,
    range: validRange
  }, fixture.projectRoot);
  assert.equal(location, null);
});

test('rejects an in-project symlink whose real target is outside the project', async t => {
  const fixture = await createFixture(t);
  const symlinkPath = path.join(fixture.sourceRoot, 'external.nx');
  try {
    await fs.symlink(fixture.outsideFile, symlinkPath);
  } catch (error) {
    if (['EPERM', 'EACCES', 'ENOSYS', 'ENOTSUP'].includes(error.code)) {
      t.skip('host does not permit symbolic links');
      return;
    }
    throw error;
  }

  const location = await toProjectLocation({
    uri: pathToFileURL(symlinkPath).href,
    range: validRange
  }, fixture.projectRoot);
  assert.equal(location, null);
});
