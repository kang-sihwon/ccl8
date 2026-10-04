'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { resolveRuntime, checkEnginePaths, enableManagedWiring } = require('../runtime.cjs');

function fixture(t) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ccl8-runtime-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  const pkg = path.join(root, '.local/graft-runtime/node_modules/@nanonets/graft');
  fs.mkdirSync(path.join(pkg, 'dist'), { recursive: true });
  fs.writeFileSync(path.join(pkg, 'package.json'), '{}');
  fs.writeFileSync(path.join(pkg, 'dist/engine.js'), '');
  return { root, pkg };
}

test('local runtime resolves relative to the repository, independently of cwd', t => {
  const { root, pkg } = fixture(t);
  fs.writeFileSync(path.join(root, '.local/agent-paths.json'), JSON.stringify({ indexRoot: './indexes' }));
  const runtime = resolveRuntime(root, {});
  assert.equal(runtime.graftPackageRoot, pkg);
  assert.equal(runtime.indexRoot, path.join(root, 'indexes'));
});

test('environment selects engine and indexes without changing saved settings', t => {
  const { root } = fixture(t);
  const file = path.join(root, '.local/agent-paths.json');
  const saved = JSON.stringify({ engineRoot: 'saved-engine', indexRoot: 'saved-indexes' });
  fs.writeFileSync(file, saved);
  const runtime = resolveRuntime(root, { UE58_ENGINE_ROOT: 'other-engine', UE58_GRAFT_INDEX_ROOT: 'other-indexes' });
  assert.equal(runtime.engineRoot, path.join(root, 'other-engine'));
  assert.equal(runtime.indexRoot, path.join(root, 'other-indexes'));
  assert.equal(fs.readFileSync(file, 'utf8'), saved);
});

test('missing engine or indexes fails without creating either', t => {
  const { root } = fixture(t);
  const runtime = resolveRuntime(root, {});
  runtime.engineRoot = path.join(root, 'absent-engine');
  assert.throws(() => checkEnginePaths(runtime), /engineRoot/);
  fs.mkdirSync(path.join(runtime.engineRoot, 'Build'), { recursive: true });
  fs.writeFileSync(path.join(runtime.engineRoot, 'Build/Build.version'), '{}');
  assert.throws(() => checkEnginePaths(runtime), /indexRoot/);
});

test('unmanaged upstream upkeep is rejected before agent startup', t => {
  const { root, pkg } = fixture(t);
  fs.writeFileSync(path.join(pkg, 'dist/upkeep-run.js'), 'export function runUpkeep() {}');
  assert.throws(() => enableManagedWiring(resolveRuntime(root, {})), /Managed wiring guard missing/);
});
