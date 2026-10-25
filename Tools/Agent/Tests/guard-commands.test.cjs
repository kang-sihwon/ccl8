'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const { decide } = require('../guard-commands.cjs');

const asks = command => decide({ tool_name: 'Bash', tool_input: { command } })?.hookSpecificOutput.permissionDecision === 'ask';

test('the hook process asks before git reset --hard and stays silent for git status', () => {
  const run = command => spawnSync(process.execPath, [path.join(__dirname, '..', 'guard-commands.cjs')],
    { input: JSON.stringify({ tool_name: 'Bash', tool_input: { command } }), encoding: 'utf8' });
  const reset = run('git reset --hard HEAD~1');
  assert.equal(reset.status, 0);
  assert.equal(JSON.parse(reset.stdout).hookSpecificOutput.permissionDecision, 'ask');
  const status = run('git status --short');
  assert.equal(status.status, 0);
  assert.equal(status.stdout, '');
});

test('destructive commands ask', () => {
  for (const command of ['git -C repo reset --hard', 'git clean -fd', 'git checkout -- Source/A.cpp', 'git checkout .',
    'git restore Docs/Guide.md', 'git push --force origin main', 'git push origin :old', 'git branch -D topic',
    'git tag -d agent-setup-v1', 'git stash drop', 'git rm Docs/Old.md', 'rm -rf build', 'cd x && rm a.txt',
    'Remove-Item -Recurse out', 'del /q file.txt', 'find . -name "*.tmp" -delete']) {
    assert.ok(asks(command), command);
  }
});

test('read-only and additive commands pass', () => {
  for (const command of ['git status', 'git log --oneline -5', 'git diff HEAD', 'git checkout -b topic', 'git checkout main',
    'git restore --staged Docs/Guide.md', 'git push origin main', 'git branch topic', 'git tag -a v1 -m x',
    'git stash list', 'node Tools/Agent/skills.cjs check', 'ls -la', 'grep -rn "rm -rf" Docs', 'Get-ChildItem']) {
    assert.ok(!asks(command), command);
  }
});

test('payloads without a command pass', () => {
  assert.equal(decide({ tool_name: 'Read', tool_input: { file_path: 'x' } }), null);
});
