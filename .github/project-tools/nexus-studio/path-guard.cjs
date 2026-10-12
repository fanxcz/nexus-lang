const path = require('node:path');
const fs = require('node:fs/promises');

function isInside(root, target) {
  const rel = path.relative(root, path.resolve(target));
  return rel === '' || (!rel.startsWith('..' + path.sep) && rel !== '..' && !path.isAbsolute(rel));
}

async function assertInsideProject(projectRoot, target) {
  if (!projectRoot) throw new Error('Сначала открой папку проекта.');

  const resolved = path.resolve(target);
  if (!isInside(projectRoot, resolved)) throw new Error('Файл находится за пределами проекта.');

  const realRoot = await fs.realpath(projectRoot);
  let probe = resolved;
  while (true) {
    let realProbe;
    try {
      realProbe = await fs.realpath(probe);
    } catch (error) {
      if (error.code !== 'ENOENT' && error.code !== 'ENOTDIR') throw error;
      const parent = path.dirname(probe);
      if (parent === probe) throw new Error('Не удалось проверить путь внутри проекта.');
      probe = parent;
      continue;
    }

    if (!isInside(realRoot, realProbe)) throw new Error('Файл находится за пределами проекта.');
    return resolved;
  }
}

module.exports = { isInside, assertInsideProject };
