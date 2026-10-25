'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { parseFrontmatter, checkRepository } = require('../skills.cjs');

function repository(t, skills) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ccl8-skills-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  for (const [file, text] of Object.entries(skills)) {
    fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
    fs.writeFileSync(path.join(root, file), text);
  }
  return root;
}

const valid = '---\nname: alpha\ndescription: Use when testing.\n---\n\nSee `references/notes.md`.\n';

test('frontmatter folds indented continuation lines and strips quotes', () => {
  const fields = parseFrontmatter('---\nname: "alpha"\ndescription: first\n  second\nmetadata:\n  author: x\n---\n');
  assert.equal(fields.name, 'alpha');
  assert.equal(fields.description, 'first second');
});

test('identical trees with valid skills pass', t => {
  const files = { 'alpha/SKILL.md': valid, 'alpha/references/notes.md': 'notes' };
  const root = repository(t, Object.fromEntries(Object.entries(files).flatMap(([file, text]) =>
    [[`.agents/skills/${file}`, text], [`.claude/skills/${file}`, text.replace(/\n/g, '\r\n')]])));
  assert.deepEqual(checkRepository(root), []);
});

test('missing trigger, missing path and a mirror-only skill are errors', t => {
  const root = repository(t, {
    '.agents/skills/alpha/SKILL.md': '---\nname: alpha\ndescription: Does things.\n---\n\nSee `references/absent.md`.\n',
    '.claude/skills/alpha/SKILL.md': '---\nname: alpha\ndescription: Does things.\n---\n\nSee `references/absent.md`.\n',
    '.claude/skills/beta/SKILL.md': '---\nname: beta\ndescription: Use when testing.\n---\n',
  });
  const messages = checkRepository(root).filter(finding => finding.level === 'error').map(finding => finding.message);
  assert.ok(messages.some(message => /when to use/.test(message)));
  assert.ok(messages.some(message => /missing path "references\/absent.md"/.test(message)));
  assert.ok(messages.some(message => /exists only here/.test(message)));
});

test('a changed mirror file is reported against the canonical tree', t => {
  const root = repository(t, {
    '.agents/skills/alpha/SKILL.md': valid,
    '.agents/skills/alpha/references/notes.md': 'notes',
    '.claude/skills/alpha/SKILL.md': valid.replace('testing', 'other work'),
    '.claude/skills/alpha/references/notes.md': 'notes',
  });
  const findings = checkRepository(root);
  assert.equal(findings.length, 1);
  assert.match(findings[0].message, /differs from \.agents\/skills/);
});

test('a user-invoked skill needs the Codex policy file and the reverse', t => {
  const hidden = 'interface:\n  display_name: "Alpha"\npolicy:\n  allow_implicit_invocation: false\n';
  const flagged = '---\nname: alpha\ndescription: Use when testing.\ndisable-model-invocation: true\n---\n';
  const plain = '---\nname: beta\ndescription: Use when testing.\n---\n';
  const root = repository(t, {
    '.agents/skills/alpha/SKILL.md': flagged,
    '.agents/skills/beta/SKILL.md': plain,
    '.agents/skills/beta/agents/openai.yaml': hidden,
    '.claude/skills/alpha/SKILL.md': flagged,
    '.claude/skills/beta/SKILL.md': plain,
    '.claude/skills/beta/agents/openai.yaml': hidden,
  });
  const messages = checkRepository(root).map(finding => finding.message);
  assert.ok(messages.some(message => /does not set allow_implicit_invocation/.test(message)));
  assert.ok(messages.some(message => /disable-model-invocation is not true/.test(message)));

  fs.mkdirSync(path.join(root, '.agents/skills/alpha/agents'), { recursive: true });
  fs.writeFileSync(path.join(root, '.agents/skills/alpha/agents/openai.yaml'), hidden);
  fs.mkdirSync(path.join(root, '.claude/skills/alpha/agents'), { recursive: true });
  fs.writeFileSync(path.join(root, '.claude/skills/alpha/agents/openai.yaml'), hidden);
  fs.writeFileSync(path.join(root, '.agents/skills/beta/SKILL.md'), flagged.replace('alpha', 'beta'));
  assert.ok(!checkRepository(root).some(finding => /invocation/.test(finding.message)));
});

test('a tool-managed skill may live only in the mirror and exceed the line budget', t => {
  const long = `---\nname: graphify\ndescription: Use when testing.\n---\n${'line\n'.repeat(600)}`;
  const root = repository(t, { '.claude/skills/graphify/SKILL.md': long });
  assert.deepEqual(checkRepository(root), []);
});
