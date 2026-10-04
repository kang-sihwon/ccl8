'use strict';
// Project-owned integration: upstream upkeep may otherwise overwrite both local
// shims and global agent settings when the generated cache stamp is missing.
const fs = require('node:fs');
const path = require('node:path');
const { resolveRuntime } = require('./runtime.cjs');
const mode = process.argv[2] || 'check';
const runtime = resolveRuntime();
const file = path.join(runtime.graftPackageRoot, 'dist', 'upkeep-run.js');
const original = 'export function runUpkeep(repo, current, opts = {}) {';
const managed = original + '\n    // ccl8 managed wiring: never rewrite agent settings or run background upkeep.\n    if (process.env.GRAFT_MANAGED_WIRING === "1") return { lines: [] };';
const version = JSON.parse(fs.readFileSync(path.join(runtime.graftPackageRoot, 'package.json'), 'utf8')).version;
if (version !== '0.18.0') throw new Error('Managed wiring patch requires Graft 0.18.0.');
const source = fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
if (mode === 'apply') {
  if (!source.includes(managed)) {
    if (source.split(original).length !== 2) throw new Error('Unexpected upkeep source; inspect before patching.');
    fs.writeFileSync(file, source.replace(original, managed));
  }
  console.log('Project-managed wiring guard applied.');
} else if (mode === 'check') {
  if (!source.includes(managed)) throw new Error('Managed wiring guard missing; run configure_runtime.cjs apply before starting agents.');
  console.log('Project-managed wiring guard verified.');
} else {
  throw new Error('Usage: node Tools/Agent/configure_runtime.cjs apply|check');
}
