'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { scan } = require('../floor-guard.cjs');

const diff = (file, hunk) => `diff --git a/${file} b/${file}\n--- a/${file}\n+++ b/${file}\n@@ -1,4 +1,4 @@\n${hunk}\n`;
const rules = text => scan(text).map(finding => finding.rule);

test('each floor move in the sample diff is caught', () => {
  const cases = {
    suppression: diff('Source/CCL/A.cpp', '+PRAGMA_DISABLE_DEPRECATION_WARNINGS'),
    'disabled-test': diff('Source/CCL/Tests/ATests.cpp', '+    EAutomationTestFlags::Disabled | EAutomationTestFlags::EngineFilter)'),
    'deleted-test': diff('Source/CCL/Tests/ATests.cpp', '-IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLATest, "CCL.A", Flags)'),
    'deleted-assertion': diff('Tools/Validation/run_a_smoke.ps1', '-    if (-not $ok) { Write-Error "A failed"; exit 1 }'),
    'timeout-increase': diff('Tools/Validation/run_a_smoke.ps1', '-param([int]$TimeoutSeconds = 180)\n+param([int]$TimeoutSeconds = 600)'),
    stub: diff('Source/CCL/A.cpp', '+    checkNoEntry();'),
  };
  for (const [rule, text] of Object.entries(cases)) assert.deepEqual(rules(text), [rule], rule);
  assert.deepEqual(rules(diff('Source/CCL/A.cpp', '+    // TODO: real damage')), ['stub']);
  assert.deepEqual(rules(diff('Tools/Agent/Tests/a.test.cjs', "+test.skip('a', () => {});")), ['disabled-test']);
});

test('moved tests and assertions, lowered timeouts and docs are not findings', () => {
  const moved = diff('Source/CCL/Tests/ATests.cpp', '-IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLATest, "CCL.A", Flags)\n-    TestTrue(TEXT("ok"), bOk);')
    + diff('Source/CCL/Tests/BTests.cpp', '+IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLATest, "CCL.A", Flags)\n+    TestTrue(TEXT("ok"), bOk);');
  assert.deepEqual(scan(moved), []);
  const split = diff('Source/CCL/Tests/ATests.cpp', '-\tif (!bOk) { AddError(Error); return false; }\n+\tif (!bOk)\n+\t{\n+\t\tAddError(Error);\n+\t\treturn false;\n+\t}');
  assert.deepEqual(scan(split), []);
  const elsewhere = diff('Source/CCL/Tests/ATests.cpp', '-\tif (!bOk) { AddError(Error); return false; }')
    + diff('Source/CCL/Tests/BTests.cpp', '+\tif (!bOk)\n+\t{\n+\t\tAddError(Error);\n+\t\treturn false;\n+\t}');
  assert.deepEqual(rules(elsewhere), ['deleted-assertion']);
  assert.deepEqual(scan(diff('Tools/Validation/run_a_smoke.ps1', '-$TimeoutSeconds = 600\n+$TimeoutSeconds = 180')), []);
  assert.deepEqual(scan(diff('Docs/Work/README.md', '+- TODO: write the map')), []);
});

test('hunk content that looks like a file header stays content', () => {
  const text = 'diff --git a/Source/A.cpp b/Source/A.cpp\n--- a/Source/A.cpp\n+++ b/Source/A.cpp\n@@ -1 +1,2 @@\n+++Counter;\n+// FIXME later\n';
  const findings = scan(text);
  assert.equal(findings.length, 1);
  assert.equal(findings[0].file, 'Source/A.cpp');
  assert.equal(findings[0].line, 2);
});
