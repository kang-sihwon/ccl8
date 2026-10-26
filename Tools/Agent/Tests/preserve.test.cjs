'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { extract, compare } = require('../preserve.cjs');

const doc = (path, text) => ({ path, text });
const tokens = (category, path, text) => [...extract(path, text, []).get(category).keys()];
const missing = (old, fresh, options) => compare(old, fresh, options).map(item => `${item.category} ${item.token}`);

test('each category is extracted once and kept apart', () => {
  const text = [
    '결정 8·10·11과 [결정 2](decisions/0002-x.md)를 따른다. 지연은 120ms, 화면 1280x720, 비율 61%, 날짜 2026-10-12, UE 5.8.1.',
    '스크립트는 `Tools/Validation/run_network_smoke.ps1`이고 `-Port 18791`을 쓴다. ID `B4A22782`.',
    '```',
    'const Speed = 500.f; // [link](ignored.md)',
    '```',
  ].join('\n');
  assert.deepEqual(tokens('decision', 'Docs/A.md', text), ['결정 8', '결정 10', '결정 11', '결정 2']);
  assert.deepEqual(tokens('link', 'Docs/A.md', text), ['Docs/decisions/0002-x.md']);
  assert.deepEqual(tokens('path', 'Docs/A.md', text), ['Tools/Validation/run_network_smoke.ps1']);
  assert.deepEqual(tokens('number', 'Docs/A.md', text), ['120ms', '1280x720', '61%', '2026-10-12', '5.8.1', '18791', '500']);
});

test('a rewrite that keeps everything reports nothing, even when moved and relinked', () => {
  const old = [doc('Docs/Plan.md', '근거는 [결정 3](DesignLog.md)이고 [Coding](Coding.md#헤더-배치)의 `helpers/`를 쓴다. 3-4개월.')];
  const fresh = [
    doc('Docs/QA/Plan.md', '3-4개월이다. 근거는 [결정 3](../decisions/0003-x.md)이다.'),
    doc('Docs/Tools.md', '[Coding](Coding.md#헤더-배치)의 `.claude/helpers/`.'),
  ];
  assert.deepEqual(missing(old, fresh, { renames: [['Docs/DesignLog.md', 'Docs/decisions/0003-x.md']] }), []);
});

test('dropped numbers, decisions, paths and anchors are reported with their first location', () => {
  const old = [doc('Docs/Plan.md', '첫 줄\n결정 5에 따라 60배, `Source/CCL/` 아래, [절](Other.md#a)과 [문서](Other.md).')];
  const fresh = [doc('Docs/Plan.md', '[문서](Other.md#b)만 남았다.')];
  const result = compare(old, fresh);
  assert.deepEqual(result.map(item => `${item.category} ${item.token}`), ['decision 결정 5', 'number 60', 'path Source/CCL/', 'link Docs/Other.md#a']);
  assert.ok(result.every(item => item.where === 'Docs/Plan.md:2'));
});

test('a link into the new set itself counts as kept after a merge', () => {
  const old = [doc('Docs/Log.md', '절차는 [가이드](Guide.md)를 본다.'), doc('Docs/Index.md', '이력은 [Log](Log.md)에 있다.')];
  const fresh = [doc('Docs/Index.md', '절차는 [가이드](Guide.md)를 본다. 이력도 여기 있다.')];
  assert.deepEqual(missing(old, fresh, { renames: [['Docs/Log.md', 'Docs/Index.md']] }), []);
});

test('allowed removals are not reported', () => {
  const old = [doc('Docs/Plan.md', '결정 5와 60배.')];
  const fresh = [doc('Docs/Plan.md', '없음.')];
  const allowed = new Set(['decision\t결정 5', 'number\t60']);
  assert.deepEqual(missing(old, fresh, { allowed }), []);
});
