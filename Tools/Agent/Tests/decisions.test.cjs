'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { loadDecisions, render, main } = require('../decisions.cjs');

function decision(number, extra = '') {
  return `---\nnumber: ${number}\ndate: 2026-10-0${number}\ntitle: "제목 ${number}"\ntopic: "주제"\nstatus: "유효"\nsummary: "요약 ${number}"\nowners: "[Coding](../Coding.md)"\n${extra}---\n\n# 2026-10-0${number} — 결정 ${number}: 제목 ${number}\n\n### 배경\n\n본문.\n`;
}

function repository(t, files) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ccl8-decisions-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  for (const [file, text] of Object.entries(files)) {
    fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
    fs.writeFileSync(path.join(root, file), text);
  }
  execFileSync('git', ['init', '-q'], { cwd: root });
  return root;
}

function check(root) {
  const lines = [];
  const write = process.stdout.write;
  process.stdout.write = chunk => lines.push(String(chunk));
  try {
    return { code: main(['check'], root), output: lines.join('') };
  } finally {
    process.stdout.write = write;
  }
}

test('relations render on both decisions and the index matches the generator', t => {
  const root = repository(t, {
    'Docs/decisions/0001-first.md': decision(1),
    'Docs/decisions/0002-second.md': decision(2, 'amends: [1]\n'),
  });
  const { decisions, findings } = loadDecisions(root);
  assert.deepEqual(findings, []);
  const table = render(decisions);
  assert.match(table, /\[결정 1\]\(decisions\/0001-first\.md\).*결정 2에서 보완 \| \[Coding\]\(Coding\.md\)/);
  assert.match(table, /\[결정 2\]\(decisions\/0002-second\.md\).*결정 1 보완/);
  fs.writeFileSync(path.join(root, 'Docs/Decisions.md'), table);
  fs.writeFileSync(path.join(root, 'Docs/Coding.md'), '근거는 [결정 2](decisions/0002-second.md)다.\n');
  assert.equal(check(root).code, 0);
});

test('bad numbering, relation targets, status and stale index are reported', t => {
  const root = repository(t, {
    'Docs/decisions/0001-first.md': decision(1, 'supersedes: [2]\n').replace('"유효"', '"보류"'),
    'Docs/decisions/0003-third.md': decision(3),
    'Docs/Decisions.md': 'old\n',
  });
  const { code, output } = check(root);
  assert.equal(code, 1);
  assert.match(output, /status "보류"/);
  assert.match(output, /not an earlier decision/);
  assert.match(output, /expected decision 2/);
  assert.match(output, /differs from the generated table/);
});

test('links to the retired log or to the wrong decision file are reported', t => {
  const root = repository(t, {
    'Docs/decisions/0001-first.md': decision(1),
    'Docs/decisions/0002-second.md': decision(2),
    'Docs/Plan.md': '[결정 1](decisions/0002-second.md), [결정 2](DesignLog.md), [결정 3](decisions/0003-none.md)\n',
  });
  const { decisions } = loadDecisions(root);
  fs.writeFileSync(path.join(root, 'Docs/Decisions.md'), render(decisions));
  const { output } = check(root);
  assert.match(output, /"결정 1" links to decision 2/);
  assert.match(output, /retired Docs\/DesignLog\.md/);
  assert.match(output, /missing decision file/);
});
