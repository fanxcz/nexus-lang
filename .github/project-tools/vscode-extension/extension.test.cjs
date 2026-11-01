const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const root = __dirname;
const readJson = relative => JSON.parse(fs.readFileSync(path.join(root, relative), 'utf8'));
const manifest = readJson('package.json');

test('the extension registers .nx as the NEXUS language', () => {
  const language = manifest.contributes.languages.find(item => item.id === 'nexus');
  assert.ok(language, 'missing NEXUS language contribution');
  assert.ok(language.extensions.includes('.nx'), 'the .nx extension is not registered');
  assert.equal(language.configuration, './language-configuration.json');
});

test('the TextMate grammar matches the declared source scope and has core syntax rules', () => {
  const contribution = manifest.contributes.grammars.find(item => item.language === 'nexus');
  assert.ok(contribution, 'missing NEXUS grammar contribution');
  const grammar = readJson(contribution.path.replace(/^\.\//, ''));
  assert.equal(grammar.scopeName, contribution.scopeName);
  assert.ok(Array.isArray(grammar.patterns) && grammar.patterns.length > 0);
  const scopes = new Set(grammar.patterns.map(pattern => pattern.name));
  for (const required of [
    'comment.line.double-slash.nexus',
    'comment.block.nexus',
    'string.quoted.double.nexus',
    'constant.numeric.nexus',
    'keyword.declaration.function.nexus',
    'keyword.control.nexus',
    'support.type.nexus',
    'keyword.operator.nexus'
  ]) {
    assert.ok(scopes.has(required), 'missing grammar rule: ' + required);
  }
});

test('language configuration defines comments, bracket pairs and indentation', () => {
  const config = readJson('language-configuration.json');
  assert.equal(config.comments.lineComment, '//');
  assert.deepEqual(config.comments.blockComment, ['/*', '*/']);
  assert.ok(config.brackets.some(pair => pair[0] === '{' && pair[1] === '}'));
  assert.ok(config.autoClosingPairs.some(pair => pair.open === '(' && pair.close === ')'));
  assert.ok(config.indentationRules.increaseIndentPattern);
  assert.ok(config.indentationRules.decreaseIndentPattern);
});

test('snippets have valid prefixes and non-empty insertion bodies', () => {
  const snippets = readJson('snippets/nexus.json');
  assert.ok(Object.keys(snippets).length > 0);
  for (const [name, snippet] of Object.entries(snippets)) {
    assert.ok(typeof snippet.prefix === 'string' && snippet.prefix.length > 0, name + ': missing prefix');
    assert.ok(Array.isArray(snippet.body) && snippet.body.length > 0, name + ': missing body');
    assert.ok(snippet.body.every(line => typeof line === 'string'), name + ': body lines must be strings');
  }
  assert.ok(Object.values(snippets).some(snippet => snippet.prefix === 'main'));
});

test('the language client starts the configured NEXUS LSP command', () => {
  const source = fs.readFileSync(path.join(root, manifest.main), 'utf8');
  assert.match(source, /get\(['"]serverPath['"],\s*['"]nexus['"]\)/);
  assert.match(source, /args:\s*\[['"]lsp['"]\]/);
  assert.match(source, /new LanguageClient/);
  assert.match(source, /documentSelector:[\s\S]*language:\s*['"]nexus['"]/);
  assert.equal(manifest.contributes.configuration.properties['nexus.serverPath'].type, 'string');
});
