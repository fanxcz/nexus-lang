const fs = require('node:fs/promises');
const path = require('node:path');
const { fileURLToPath } = require('node:url');
const { assertInsideProject } = require('./path-guard.cjs');

async function toProjectLocation(location, projectRoot) {
  if (!location || typeof location !== 'object') return null;
  const targetUri = typeof location.uri === 'string'
    ? location.uri
    : location.targetUri;
  if (typeof targetUri !== 'string' || !targetUri.startsWith('file:')) return null;

  const range = location.range || location.targetSelectionRange || location.targetRange;
  if (!range || !range.start || !range.end ||
      !Number.isSafeInteger(range.start.line) || range.start.line < 0 ||
      !Number.isSafeInteger(range.start.character) || range.start.character < 0 ||
      !Number.isSafeInteger(range.end.line) || range.end.line < 0 ||
      !Number.isSafeInteger(range.end.character) || range.end.character < 0) {
    return null;
  }

  try {
    const targetPath = fileURLToPath(targetUri);
    const realTarget = await fs.realpath(targetPath);
    const checkedTarget = await assertInsideProject(projectRoot, realTarget);
    if (path.extname(checkedTarget).toLowerCase() !== '.nx') return null;

    return {
      path: path.relative(projectRoot, checkedTarget).split(path.sep).join('/'),
      range
    };
  } catch {
    // Malformed, missing, or out-of-project targets are not navigable in Studio.
    return null;
  }
}

module.exports = { toProjectLocation };
