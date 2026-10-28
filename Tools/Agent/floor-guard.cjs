#!/usr/bin/env node
'use strict';

// Diff-scoped floor check: reports changes that lower the bar instead of meeting it.
// Usage: node Tools/Agent/floor-guard.cjs [--base <rev>]   (default HEAD; untracked files are included)
// Exit codes: 0 clean, 1 at least one finding, 2 the guard could not run.

const { execFileSync } = require('node:child_process');

const CODE_FILE = /\.(h|hpp|cpp|cs|py|ps1|psm1|cmd|bat|cjs|mjs|js|ts)$/i;
const TEST_FILE = /(^|\/)(Tests?|Validation)\/|Smoke|Tests?\.\w+$|\.test\.\w+$/i;
// The guard and its tests spell out every pattern they look for.
const SELF = new Set(['Tools/Agent/floor-guard.cjs', 'Tools/Agent/Tests/floor-guard.test.cjs']);

const SUPPRESSION = /\bNOLINT|#\s*pragma\s+warning\s*\(\s*disable|PRAGMA_DISABLE_\w*WARNINGS|UE_DISABLE_OPTIMIZATION|PRAGMA_DISABLE_OPTIMIZATION|#\s*pragma\s+optimize\s*\(\s*""\s*,\s*off|#\s*pragma\s+clang\s+diagnostic\s+ignored|clang-format\s+off|eslint-disable|#\s*noqa|#\s*type:\s*ignore|@ts-(ignore|nocheck)|SuppressMessage/;
const DISABLED_TEST = /EAutomationTestFlags::Disabled|\bDISABLED_\w+|\b(test|it|describe)\.(skip|only)\s*\(|\bt\.skip\s*\(|\{\s*skip\s*:|@unittest\.skip|pytest\.mark\.skip|^\s*#\s*if\s+0\b/;
const TEST_DEFINITION = /\b(?:IMPLEMENT_(?:CUSTOM_)?(?:SIMPLE|COMPLEX)_AUTOMATION_TEST|BEGIN_DEFINE_SPEC|DEFINE_SPEC)\s*\(\s*(\w+)|^\s*test\s*\(\s*(['"`][^'"`]+['"`])|^\s*def\s+(test_\w+)/;
const ASSERTION = /\b(?:Test(?:True|False|Equal|NotEqual|Null|NotNull|Valid|Invalid|Same|NotSame|NearlyEqual)|AddError|UTEST_\w+|assert(?:\.\w+)?|Write-Error|Fail)\s*\(|\bthrow\b|\bexit\s+[1-9]/;
const TIMEOUT = /(\w*(?:Timeout|TimeLimit|Deadline)\w*)\s*[=:]\s*(\d+(?:\.\d+)?)/i;
const STUB = /\bcheckNoEntry\s*\(|\bunimplemented\s*\(|\b(?:TODO|FIXME|XXX|HACK)\b|NotImplementedError|not implemented/i;

function git(args, allowDiffExit = false) {
  try {
    return execFileSync('git', args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'], maxBuffer: 256 * 1024 * 1024 });
  } catch (error) {
    // `git diff --no-index` exits 1 whenever the sides differ, which is the normal case for a new file.
    if (allowDiffExit && error.status === 1 && typeof error.stdout === 'string') return error.stdout;
    return null;
  }
}

function parseDiff(diff) {
  const added = [];
  const removed = [];
  let file = '';
  let oldFile = '';
  let header = false;
  let line = 0;
  for (const text of diff.split(/\r?\n/)) {
    if (text.startsWith('diff ')) header = true;
    else if (text.startsWith('@@')) {
      header = false;
      line = Number((text.match(/\+(\d+)/) || [0, 0])[1]);
    } else if (header) {
      // Only header lines between `diff` and the first hunk name files; inside a hunk `+++i` is content.
      if (text.startsWith('--- ')) oldFile = text.slice(4).replace(/^[ab]\//, '');
      else if (text.startsWith('+++ ')) {
        const name = text.slice(4).replace(/^[ab]\//, '');
        file = name === '/dev/null' ? oldFile : name;
      }
    } else if (text.startsWith('+')) added.push({ file, line: line++, text: text.slice(1) });
    else if (text.startsWith('-')) removed.push({ file, line, text: text.slice(1) });
    else if (text.startsWith(' ')) line++;
  }
  return { added, removed };
}

function scan(diff) {
  const { added, removed } = parseDiff(diff);
  const inScope = entry => CODE_FILE.test(entry.file) && !SELF.has(entry.file);
  const plus = added.filter(inScope);
  const minus = removed.filter(inScope);
  const findings = [];
  const report = (rule, entry, message) => findings.push({ rule, file: entry.file, line: entry.line, message });

  for (const entry of plus) {
    if (SUPPRESSION.test(entry.text)) report('suppression', entry, 'new suppression of a checker or optimizer');
    if (TEST_FILE.test(entry.file) && DISABLED_TEST.test(entry.text)) report('disabled-test', entry, 'test disabled, skipped or narrowed');
    if (STUB.test(entry.text)) report('stub', entry, 'unfinished-work marker or unreachable stub');
  }

  // A definition or assertion that reappears elsewhere in the diff was moved, not removed.
  // Splitting a one-line block over several lines also keeps it, so each file's added lines
  // are joined without whitespace and searched for the removed line.
  const normalize = text => text.replace(/\s+/g, ' ').trim();
  const addedTexts = new Map();
  for (const entry of plus) addedTexts.set(normalize(entry.text), (addedTexts.get(normalize(entry.text)) ?? 0) + 1);
  const strip = text => text.replace(/\s+/g, '');
  const addedJoined = new Map();
  for (const entry of plus) addedJoined.set(entry.file, (addedJoined.get(entry.file) ?? '') + strip(entry.text));
  const addedTests = new Set(plus.map(entry => entry.text.match(TEST_DEFINITION)).filter(Boolean).map(match => match.slice(1).find(Boolean)));
  for (const entry of minus) {
    const definition = entry.text.match(TEST_DEFINITION);
    if (definition && !addedTests.has(definition.slice(1).find(Boolean))) {
      report('deleted-test', entry, `test ${definition.slice(1).find(Boolean)} removed`);
      continue;
    }
    if (TEST_FILE.test(entry.file) && ASSERTION.test(entry.text)) {
      const key = normalize(entry.text);
      const count = addedTexts.get(key) ?? 0;
      if (count > 0) addedTexts.set(key, count - 1);
      else if (!(addedJoined.get(entry.file) ?? '').includes(strip(entry.text))) report('deleted-assertion', entry, 'assertion or failure exit removed from a test or smoke');
    }
  }

  // A timeout raised in place shows as a removed and an added line with the same key in one file.
  const before = new Map();
  for (const entry of minus) {
    const match = entry.text.match(TIMEOUT);
    if (match) before.set(`${entry.file}\0${match[1].toLowerCase()}`, Number(match[2]));
  }
  for (const entry of plus) {
    const match = entry.text.match(TIMEOUT);
    const old = match && before.get(`${entry.file}\0${match[1].toLowerCase()}`);
    if (old !== undefined && old !== null && Number(match[2]) > old) report('timeout-increase', entry, `${match[1]} raised from ${old} to ${match[2]}`);
  }
  return findings;
}

function collectDiff(base) {
  const top = git(['rev-parse', '--show-toplevel'])?.trim();
  if (!top) throw new Error('not inside a git work tree');
  process.chdir(top);
  if (git(['rev-parse', '--verify', `${base}^{commit}`]) === null) throw new Error(`unknown base ${base}`);
  const tracked = git(['diff', '--unified=0', '--no-color', base, '--']);
  if (tracked === null) throw new Error(`could not diff against ${base}`);
  const untracked = git(['ls-files', '--others', '--exclude-standard']);
  if (untracked === null) throw new Error('could not list untracked files');
  const extra = untracked.split('\n').filter(file => file && CODE_FILE.test(file)).map(file => {
    const diff = git(['diff', '--no-index', '--unified=0', '--no-color', '/dev/null', file], true);
    if (diff === null) throw new Error(`could not diff untracked file ${file}`);
    return diff;
  });
  return [tracked, ...extra].join('\n');
}

if (require.main === module) {
  try {
    const index = process.argv.indexOf('--base');
    const findings = scan(collectDiff(index > 0 ? process.argv[index + 1] : 'HEAD'));
    for (const { rule, file, line, message } of findings) process.stdout.write(`${rule} ${file}:${line} ${message}\n`);
    process.stdout.write(`floor-guard: ${findings.length} finding(s)\n`);
    process.exitCode = findings.length ? 1 : 0;
  } catch (error) {
    process.stderr.write(`floor-guard: ${error.message}\n`);
    process.exitCode = 2;
  }
}

module.exports = { parseDiff, scan };
