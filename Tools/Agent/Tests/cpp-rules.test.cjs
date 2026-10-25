'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { checkSource, clangFormat } = require('../cpp-rules.cjs');

const fixtures = path.join(__dirname, 'fixtures', 'cpp');
const rules = folder => ['LayoutSample.h', 'LayoutSample.cpp'].flatMap(name => {
  const file = path.join(fixtures, folder, name);
  return checkSource(file, fs.readFileSync(file, 'utf8')).map(finding => finding.rule);
});

test('the fixed fixture passes every layout rule', () => {
  assert.deepEqual(rules('Good'), []);
});

test('the broken fixture fails every layout rule once or more', () => {
  const found = new Set(rules('Bad'));
  for (const rule of ['pragma-once', 'coreminimal-first', 'generated-include-last', 'generated-body-blank', 'access-blank',
    'editor-uproperty', 'editor-only-data-nested', 'blank-double', 'bool-member', 'blank-before-close', 'own-header-first',
    'cpp-one-line-function', 'blank-after-open', 'brace-own-line', 'braced-body', 'else-line']) {
    assert.ok(found.has(rule), `missing ${rule}`);
  }
});

test('keywords and braces inside comments and strings are ignored', () => {
  const text = 'void F()\n{\n\t// if (x) { y(); }\n\tLog(TEXT("if (a) b;"));\n\t/* while (z) w(); */\n}\n';
  assert.deepEqual(checkSource('Sample.cpp', text), []);
});

test('clang-format accepts the fixed fixture and rejects the broken one', () => {
  const tool = clangFormat();
  assert.ok(tool, 'clang-format not found: install the Visual Studio "C++ Clang tools" component or set CLANG_FORMAT');
  const run = file => {
    try {
      execFileSync(tool, ['--dry-run', '--Werror', '--style=file', file], { stdio: 'pipe' });
      return true;
    } catch {
      return false;
    }
  };
  assert.ok(run(path.join(fixtures, 'Good', 'LayoutSample.cpp')));
  assert.ok(!run(path.join(fixtures, 'Bad', 'LayoutSample.cpp')));
});
