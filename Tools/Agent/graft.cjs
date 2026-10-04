#!/usr/bin/env node
'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { resolveRuntime, checkEnginePaths, enableManagedWiring } = require('./runtime.cjs');

try {
  const [mode, ...args] = process.argv.slice(2);
  const runtime = resolveRuntime();
  if (mode === 'doctor') {
    checkEnginePaths(runtime);
    const version = JSON.parse(fs.readFileSync(path.join(runtime.graftPackageRoot, 'package.json'), 'utf8')).version;
    process.stdout.write(`${JSON.stringify({ ...runtime, version }, null, 2)}\n`);
  } else {
    enableManagedWiring(runtime);
    let entry;
    let childArgs;
    if (mode === 'engine') {
      checkEnginePaths(runtime);
      entry = path.join(runtime.projectRoot, 'Tools', 'Graft', 'graft_ue58_federated_mcp.cjs');
      childArgs = ['--engine-root', runtime.engineRoot, '--index-root', runtime.indexRoot, ...args];
    } else if (mode === 'project' || mode === 'cli') {
      const pkg = JSON.parse(fs.readFileSync(path.join(runtime.graftPackageRoot, 'package.json'), 'utf8'));
      const bin = typeof pkg.bin === 'string' ? pkg.bin : pkg.bin.graft;
      if (!bin) throw new Error('Installed Graft package has no graft CLI entry.');
      entry = path.resolve(runtime.graftPackageRoot, bin);
      childArgs = mode === 'project' ? ['mcp', ...args] : args;
    } else {
      throw new Error('Usage: node Tools/Agent/graft.cjs doctor|engine|project|cli [arguments]');
    }
    const child = spawn(process.execPath, [entry, ...childArgs], {
      cwd: runtime.projectRoot,
      env: { ...process.env, GRAFT_PACKAGE_ROOT: runtime.graftPackageRoot,
        UE58_ENGINE_ROOT: runtime.engineRoot, UE58_GRAFT_INDEX_ROOT: runtime.indexRoot,
        GRAFT_ONLY_EXTENSIONS: '.h,.hpp,.hh,.c,.cc,.cxx,.cpp,.cs' },
      stdio: 'inherit', windowsHide: true,
    });
    child.on('error', error => { process.stderr.write(`${error.message}\n`); process.exitCode = 1; });
    child.on('exit', code => { process.exitCode = code ?? 1; });
  }
} catch (error) {
  process.stderr.write(`${error.message}\n`);
  process.exitCode = 1;
}
