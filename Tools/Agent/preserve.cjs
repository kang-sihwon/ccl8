#!/usr/bin/env node
'use strict';

// Lists numbers, decision references, code paths and link targets of old documents that the new documents lost.
// Usage: node Tools/Agent/preserve.cjs --old <path>... --new <path>... [--base <rev>] [--rename <old>=<new>]... [--allow <file>]
// Old paths are read from <rev> (default HEAD); new paths from the working tree. Exit code 0 kept, 1 missing, 2 usage.

const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const ROOT = path.resolve(__dirname, '..', '..');
const CATEGORIES = ['decision', 'number', 'path', 'link'];
const CODE_SPAN = /`([^`\n]+)`/g;
const LINK = /\]\(([^)\s]+)\)/g;
const DECISION = /결정\s*(\d+)((?:\s*[·,]\s*\d+)*)/g;
// Dates, sizes such as 1280x720, and numbers with an optional exponent and unit (61%, 120ms, 13.5KB).
const NUMBER = /\d{4}-\d{2}-\d{2}|(?<![\w.])\d+x\d+(?![\w])|(?<![\w.])\d+(?:[.,]\d+)*(?:e[+-]?\d+)?(?:%|[a-z]+)?(?![\w])/gi;
const PATH_LIKE = /[\\/]|\.[A-Za-z][A-Za-z0-9]{0,5}$/;

const posix = file => file.split(path.sep).join('/');

// Link targets resolve against the document so a moved document keeps comparable targets.
function linkKey(docPath, target, renames) {
  if (/^[a-z][a-z0-9+.-]*:/i.test(target)) return target;
  const [file, anchor] = target.split('#');
  let resolved = file ? posix(path.normalize(path.join(path.dirname(docPath), decodeURIComponent(file)))) : docPath;
  for (const [from, to] of renames) {
    if (resolved === from) resolved = to;
  }
  return anchor === undefined ? resolved : `${resolved}#${anchor}`;
}

function extract(docPath, text, renames) {
  const tokens = new Map(CATEGORIES.map(category => [category, new Map()]));
  const add = (category, token, line) => {
    if (!tokens.get(category).has(token)) tokens.get(category).set(token, line);
  };
  let fence = false;
  text.split(/\r?\n/).forEach((raw, index) => {
    const line = index + 1;
    if (/^\s*(```|~~~)/.test(raw)) {
      fence = !fence;
      return;
    }
    let rest = raw;
    if (fence) {
      // Inside code blocks only numbers are compared; links and spans there are code, not references.
      for (const match of rest.matchAll(NUMBER)) add('number', match[0], line);
      return;
    }
    rest = rest.replace(LINK, (all, target) => {
      add('link', linkKey(docPath, target, renames), line);
      return ']';
    });
    rest = rest.replace(CODE_SPAN, (all, code) => {
      if (PATH_LIKE.test(code.trim()) && !/\s/.test(code.trim())) {
        add('path', code.trim(), line);
        return ' ';
      }
      return ` ${code} `;
    });
    rest = rest.replace(DECISION, (all, first, more) => {
      for (const number of [first, ...more.split(/[·,]/).map(item => item.trim()).filter(Boolean)]) add('decision', `결정 ${Number(number)}`, line);
      return ' ';
    });
    for (const match of rest.matchAll(NUMBER)) add('number', match[0], line);
  });
  return tokens;
}

function readOld(base, file) {
  return execFileSync('git', ['show', `${base}:${file}`], { cwd: ROOT, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
}

function readAllow(file) {
  const allowed = new Set();
  if (!file) return allowed;
  for (const raw of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
    const line = raw.replace(/\s+#.*$/, '').trim();
    if (!line) continue;
    const [category, ...rest] = line.split(/\s+/);
    if (!CATEGORIES.includes(category)) throw new Error(`allow file: unknown category "${category}"`);
    allowed.add(`${category}\t${rest.join(' ')}`);
  }
  return allowed;
}

// old: [{ path, text }], fresh: [{ path, text }]. Returns missing tokens with their first old location.
function compare(old, fresh, { renames = [], allowed = new Set() } = {}) {
  const have = new Map(CATEGORIES.map(category => [category, new Set()]));
  for (const doc of fresh) {
    for (const [category, tokens] of extract(doc.path, doc.text, renames)) {
      for (const token of tokens.keys()) have.get(category).add(token);
    }
  }
  // A link without an anchor survives as any link to the same file, and a path survives as a link to it or
  // as a longer path ending in it (`helpers/` kept as `.claude/helpers/`).
  // A link to one of the new documents themselves needs no link: its target is part of the set.
  const linkedFiles = new Set([...have.get('link')].map(token => token.split('#')[0]).concat(fresh.map(doc => doc.path)));
  const paths = [...have.get('path'), ...linkedFiles];
  const kept = (category, token) => {
    if (have.get(category).has(token)) return true;
    if (category === 'link') return !token.includes('#') && linkedFiles.has(token);
    if (category === 'path') return paths.some(candidate => candidate === token || candidate.endsWith(`/${token}`));
    return false;
  };
  const missing = [];
  const seen = new Set();
  for (const doc of old) {
    for (const [category, tokens] of extract(doc.path, doc.text, renames)) {
      for (const [token, line] of tokens) {
        const key = `${category}\t${token}`;
        if (kept(category, token) || allowed.has(key) || seen.has(key)) continue;
        seen.add(key);
        missing.push({ category, token, where: `${doc.path}:${line}` });
      }
    }
  }
  return missing;
}

function parseArgs(argv) {
  const options = { old: [], fresh: [], base: 'HEAD', renames: [], allow: null };
  let list = null;
  for (let index = 0; index < argv.length; index++) {
    const arg = argv[index];
    if (arg === '--old') list = options.old;
    else if (arg === '--new') list = options.fresh;
    else if (arg === '--base') options.base = argv[++index];
    else if (arg === '--allow') options.allow = argv[++index];
    else if (arg === '--rename') {
      const [from, to] = (argv[++index] ?? '').split('=');
      if (!from || !to) throw new Error('--rename needs <old>=<new>');
      options.renames.push([posix(from), posix(to)]);
      list = null;
    } else if (list) list.push(posix(arg));
    else throw new Error(`unexpected argument "${arg}"`);
    if (['--base', '--allow'].includes(arg)) list = null;
  }
  if (!options.old.length || !options.fresh.length) throw new Error('need at least one --old and one --new path');
  return options;
}

function main(argv) {
  let options;
  try {
    options = parseArgs(argv);
  } catch (error) {
    process.stderr.write(`${error.message}\nUsage: node Tools/Agent/preserve.cjs --old <path>... --new <path>... [--base <rev>] [--rename <old>=<new>]... [--allow <file>]\n`);
    return 2;
  }
  let missing;
  try {
    const old = options.old.map(file => ({ path: file, text: readOld(options.base, file) }));
    const fresh = options.fresh.map(file => ({ path: file, text: fs.readFileSync(path.join(ROOT, file), 'utf8') }));
    missing = compare(old, fresh, { renames: options.renames, allowed: readAllow(options.allow) });
  } catch (error) {
    process.stderr.write(`preserve: ${error.message.split('\n')[0]}\n`);
    return 2;
  }
  for (const { category, token, where } of missing) process.stdout.write(`MISSING ${category} ${token}  (${where})\n`);
  const counts = CATEGORIES.map(category => `${category} ${missing.filter(item => item.category === category).length}`).join(', ');
  process.stdout.write(`preserve: ${missing.length} missing (${counts})\n`);
  return missing.length ? 1 : 0;
}

if (require.main === module) {
  process.exitCode = main(process.argv.slice(2));
}

module.exports = { extract, compare, linkKey, main };
