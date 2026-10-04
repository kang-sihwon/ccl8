'use strict';

const fs = require('node:fs');
const path = require('node:path');
const projectRoot = path.resolve(__dirname, '../..');

function readSettings(root = projectRoot) {
  const file = path.join(root, '.local', 'agent-paths.json');
  if (!fs.existsSync(file)) return {};
  return JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));
}

function resolvePath(value, root) {
  return value ? path.resolve(root, value) : '';
}

function resolveRuntime(root = projectRoot, env = process.env) {
  const settings = readSettings(root);
  const candidates = [
    resolvePath(env.GRAFT_PACKAGE_ROOT || settings.graftPackageRoot, root),
    path.join(root, '.local', 'graft-runtime', 'node_modules', '@nanonets', 'graft'),
    env.APPDATA && path.join(env.APPDATA, 'npm', 'node_modules', '@nanonets', 'graft'),
    path.resolve(path.dirname(process.execPath), '../lib/node_modules/@nanonets/graft'),
  ].filter(Boolean);
  const graftPackageRoot = candidates.find(candidate =>
    fs.existsSync(path.join(candidate, 'package.json')) &&
    fs.existsSync(path.join(candidate, 'dist', 'engine.js')));
  if (!graftPackageRoot) {
    throw new Error('Graft runtime missing. Follow Docs/AgentContextTools.md to install it locally.');
  }
  return {
    projectRoot: root,
    graftPackageRoot,
    engineRoot: resolvePath(env.UE58_ENGINE_ROOT || settings.engineRoot || '../Engine', root),
    indexRoot: resolvePath(env.UE58_GRAFT_INDEX_ROOT || settings.indexRoot, root),
    vaultRoot: resolvePath(settings.vaultRoot, root),
  };
}

function checkEnginePaths(runtime) {
  if (!fs.existsSync(path.join(runtime.engineRoot, 'Build', 'Build.version'))) {
    throw new Error('UE engineRoot must point to the Engine folder. Run setup_local.ps1 with -EngineRoot.');
  }
  if (!runtime.indexRoot || !fs.existsSync(runtime.indexRoot)) {
    throw new Error('UE indexRoot missing. Run setup_local.ps1 with -IndexRoot; indexes are never rebuilt automatically.');
  }
}

function enableManagedWiring(runtime) {
  const source = fs.readFileSync(path.join(runtime.graftPackageRoot, 'dist', 'upkeep-run.js'), 'utf8');
  if (!source.includes('if (process.env.GRAFT_MANAGED_WIRING === "1") return { lines: [] };')) {
    throw new Error('Managed wiring guard missing. Run node Tools/Agent/configure_runtime.cjs apply.');
  }
  process.env.GRAFT_MANAGED_WIRING = '1';
}

module.exports = { projectRoot, readSettings, resolveRuntime, checkEnginePaths, enableManagedWiring };
