'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { anchors, slug, checkRepository, main } = require('../docs.cjs');

function repository(t, files) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ccl8-docs-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  for (const [file, text] of Object.entries(files)) {
    fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
    fs.writeFileSync(path.join(root, file), text);
  }
  execFileSync('git', ['init', '-q'], { cwd: root });
  return root;
}

const kinds = root => checkRepository(root).findings.map(finding => finding.kind);

test('slugs follow GitHub for Korean headings, punctuation, and code', () => {
  assert.equal(slug('단계 4 구현과 검증'), '단계-4-구현과-검증');
  assert.equal(slug('스킬 묶음 통합 검토 (2026-10-25)'), '스킬-묶음-통합-검토-2026-10-25');
  assert.equal(slug('눈·물·날씨'), '눈물날씨');
  assert.equal(slug('`Docs` Map'), 'docs-map');
});

test('repeated headings get numbered anchors and fenced headings are ignored', () => {
  const found = anchors('# 제목\n\n## 결과\n\n```text\n## 코드\n```\n\n## 결과\n');
  assert.deepEqual([...found], ['제목', '결과', '결과-1']);
});

test('a clean tree passes through a folder index', t => {
  const root = repository(t, {
    'Docs/README.md': '# 지도\n\n[하위](Sub/README.md)\n',
    'Docs/Sub/README.md': '# 하위\n\n[잎](Leaf.md#절)\n',
    'Docs/Sub/Leaf.md': '# 잎\n\n## 절\n',
  });
  assert.deepEqual(kinds(root), []);
});

test('missing files, anchors, and unreachable documents are reported', t => {
  const root = repository(t, {
    'Docs/README.md': '# 지도\n\n[없음](Gone.md) [앵커](Leaf.md#없음)\n',
    'Docs/Leaf.md': '# 잎\n',
    'Docs/Orphan.md': '# 고아\n',
  });
  assert.deepEqual(kinds(root).sort(), ['anchor', 'file', 'map']);
});

test('a directory link reaches its README or its direct documents', t => {
  const root = repository(t, {
    'Docs/README.md': '# 지도\n\n[결정](decisions/) [QA](QA/)\n',
    'Docs/decisions/0001-a.md': '# 결정\n',
    'Docs/QA/README.md': '# QA\n\n[보고서](Report.md)\n',
    'Docs/QA/Report.md': '# 보고서\n',
  });
  assert.deepEqual(kinds(root), []);
});

test('links inside code are ignored', t => {
  const root = repository(t, {
    'Docs/README.md': '# 지도\n\n`[없음](Gone.md)`\n\n```md\n[없음](Gone.md)\n```\n',
  });
  assert.deepEqual(kinds(root), []);
});

test('known broken links pass until they resolve', t => {
  const root = repository(t, { 'Docs/README.md': '# 지도\n\n[없음](Gone.md)\n' });
  const known = new Set(['Docs/README.md Gone.md']);
  assert.deepEqual(checkRepository(root, known).findings, []);
  fs.writeFileSync(path.join(root, 'Docs/Gone.md'), '# 있음\n');
  assert.deepEqual(checkRepository(root, known).findings.map(finding => finding.kind), ['known']);
});

test('the command exits by outcome and rejects unknown modes', t => {
  const root = repository(t, { 'Docs/README.md': '# 지도\n\n[없음](Gone.md)\n' });
  const write = process.stdout.write;
  process.stdout.write = () => true;
  try {
    assert.equal(main(['check'], root), 1);
  } finally {
    process.stdout.write = write;
  }
  const stderr = process.stderr.write;
  process.stderr.write = () => true;
  try {
    assert.equal(main(['bogus'], root), 2);
  } finally {
    process.stderr.write = stderr;
  }
});
