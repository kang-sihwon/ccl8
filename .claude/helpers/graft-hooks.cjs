#!/usr/bin/env node
'use strict';
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { resolveRuntime, enableManagedWiring } = require('../../Tools/Agent/runtime.cjs');
try {
  const runtime = resolveRuntime();
  enableManagedWiring(runtime);
  process.env.CLAUDE_PROJECT_DIR = runtime.projectRoot;
  const entry = path.join(runtime.graftPackageRoot, 'dist', 'claude', 'hooks.js');
  import(pathToFileURL(entry).href).then(m => m.main(process.argv[2])).catch(error => {
    process.stderr.write('ccl8 Graft: ' + error.message + '\n');
  });
} catch (error) {
  process.stderr.write('ccl8 Graft: ' + error.message + '\n');
}
