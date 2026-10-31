#!/usr/bin/env node
'use strict';

// Checks relative Markdown links, heading anchors, and that every document is reachable from the document map.
// Usage: node Tools/Agent/docs.cjs check [--summary]

const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const ROOT = path.resolve(__dirname, '..', '..');
const MAP_FILE = 'Docs/README.md';
const SCANNED = /^(Docs\/|AGENTS\.md$|README\.md$|GLOSSARY\.md$|\.agents\/skills\/)/;
// Third-party reference guides keep their upstream links; skills.cjs checks the paths skills rely on.
const NOT_SCANNED = /^\.agents\/skills\/[^/]+\/references\//;
// Records that Docs/Narrative/Workflow.md names by folder and file pattern instead of one link each.
const UNLISTED = /^Docs\/Narrative\/(Research|Drafts|Reviews)\//;
// The table in Docs/Learning/README.md#알려진-깨진-링크, fixed when those documents are rewritten (결정 28).
const KNOWN_BROKEN = new Set([
  'Docs/Learning/ContentValidation.md ../../Source/CCL/Editor/CCLCombatAssetLibrary.cpp',
  'Docs/Learning/ContentValidation.md ../../Source/CCL/Editor/CCLProgressionAssetLibrary.cpp',
  'Docs/Learning/EnvironmentFollowupEvidence.md ../Packaging.md#2026-10-11-이후-변경의-패키지-미검증',
  'Docs/Learning/EnvironmentMapConfiguration.md ../../Source/CCL/Editor/CCLEnvironmentFXLibrary.cpp',
  'Docs/Learning/InventoryUI.md ../../Source/CCL/UI/Core/CCLUISubsystem.cpp',
  'Docs/Learning/InventoryUI.md ../../Source/CCL/UI/Core/CCLScreen.cpp',
  'Docs/Learning/InventoryUI.md ../../Source/CCL/UI/Core/CCLUIRoot.cpp',
  'Docs/Learning/Map.md ../../Source/CCL/Map/CCLMapSystem.cpp',
  'Docs/Learning/UIFramework.md ../../Source/CCL/UI/Core/CCLUISubsystem.cpp',
  'Docs/Learning/UIPresentation.md ../../Source/CCL/UI/Core/CCLUISubsystem.cpp',
]);
const FENCE = /^ {0,3}(`{3,}|~{3,})/;
const HEADING = /^ {0,3}#{1,6}[ \t]+(.+?)[ \t]*#*[ \t]*$/;
const LINK = /\]\(([^)\s]+)(?:[ \t]+"[^"]*")?\)/g;
const SCHEME = /^[a-z][a-z0-9+.-]*:/i;
const INLINE_CODE = /`[^`]*`/g;

function markdownFiles(root) {
  const output = execFileSync('git', ['ls-files', '--cached', '--others', '--exclude-standard', '*.md'], { cwd: root, encoding: 'utf8' });
  return output.split(/\r?\n/).filter(file => file && SCANNED.test(file) && !NOT_SCANNED.test(file) && fs.existsSync(path.join(root, file)));
}

// Blanks fenced blocks so line numbers survive and code samples never count as headings or links.
function proseLines(text) {
  let fence = null;
  return text.split(/\r?\n/).map(line => {
    const marker = line.match(FENCE);
    if (fence) {
      if (marker && marker[1][0] === fence[0] && marker[1].length >= fence.length) fence = null;
      return '';
    }
    if (marker) {
      fence = marker[1];
      return '';
    }
    return line;
  });
}

function slug(heading) {
  const plain = heading.replace(/!?\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/`/g, '');
  return plain.trim().toLowerCase().replace(/[^\p{L}\p{M}\p{N}\p{Pc}\- ]/gu, '').replace(/ /g, '-');
}

function anchors(text) {
  const seen = new Map();
  const result = new Set();
  for (const line of proseLines(text)) {
    for (const [, id] of line.matchAll(/<a\s+(?:id|name)="([^"]+)"/g)) result.add(id);
    const match = line.match(HEADING);
    if (!match) continue;
    const base = slug(match[1]);
    const count = seen.get(base) ?? 0;
    seen.set(base, count + 1);
    result.add(count ? `${base}-${count}` : base);
  }
  return result;
}

function relative(root, full) {
  return path.relative(root, full).split(path.sep).join('/');
}

function checkRepository(root, known_broken = KNOWN_BROKEN) {
  const files = markdownFiles(root);
  const used = new Set();
  const known = new Map();
  const texts = new Map(files.map(file => [file, fs.readFileSync(path.join(root, file), 'utf8')]));
  const anchorsOf = file => {
    if (!known.has(file)) {
      const text = texts.get(file) ?? fs.readFileSync(path.join(root, file), 'utf8');
      known.set(file, anchors(text));
    }
    return known.get(file);
  };
  const findings = [];
  const edges = new Map(files.map(file => [file, new Set()]));
  for (const file of files) {
    proseLines(texts.get(file)).forEach((line, index) => {
      const where = `${file}:${index + 1}`;
      for (const [, target] of line.replace(INLINE_CODE, '').matchAll(LINK)) {
        if (SCHEME.test(target)) continue;
        const report = (kind, text) => {
          const key = `${file} ${target}`;
          if (known_broken.has(key)) used.add(key);
          else findings.push({ kind, text });
        };
        const [rawPath, rawAnchor] = target.split('#');
        let decoded;
        try {
          decoded = decodeURIComponent(rawPath);
        } catch {
          decoded = rawPath;
        }
        const full = decoded ? path.resolve(root, path.dirname(file), decoded) : path.join(root, file);
        if (!fs.existsSync(full)) {
          report('file', `${where}: missing target ${target}`);
          continue;
        }
        const resolved = relative(root, full);
        if (fs.statSync(full).isDirectory()) {
          const index = `${resolved}/README.md`;
          const listed = texts.has(index) ? [index] : files.filter(item => path.posix.dirname(item) === resolved);
          for (const item of listed) edges.get(file).add(item);
          continue;
        }
        if (!resolved.endsWith('.md')) continue;
        if (texts.has(resolved)) edges.get(file).add(resolved);
        if (rawAnchor && !anchorsOf(resolved).has(decodeURIComponent(rawAnchor).toLowerCase())) {
          report('anchor', `${where}: missing anchor ${target}`);
        }
      }
    });
  }
  for (const key of known_broken) {
    if (texts.has(key.slice(0, key.indexOf(' '))) && !used.has(key)) findings.push({ kind: 'known', text: `${key}: no longer broken, remove it from KNOWN_BROKEN` });
  }
  const reached = new Set();
  const pending = texts.has(MAP_FILE) ? [MAP_FILE] : [];
  while (pending.length) {
    const file = pending.pop();
    if (reached.has(file)) continue;
    reached.add(file);
    pending.push(...edges.get(file));
  }
  for (const file of files) {
    if (file.startsWith('Docs/') && !UNLISTED.test(file) && !reached.has(file)) findings.push({ kind: 'map', text: `${file}: not reachable from ${MAP_FILE}` });
  }
  return { files, findings };
}

function main(argv, root) {
  const [mode, ...flags] = argv;
  if (mode !== 'check') {
    process.stderr.write('Usage: node Tools/Agent/docs.cjs check [--summary]\n');
    return 2;
  }
  const { files, findings } = checkRepository(root);
  if (flags.includes('--summary')) {
    const counts = {};
    for (const { kind } of findings) counts[kind] = (counts[kind] ?? 0) + 1;
    for (const [kind, count] of Object.entries(counts)) process.stdout.write(`${kind}: ${count}\n`);
  } else {
    for (const { text } of findings) process.stdout.write(`ERROR ${text}\n`);
  }
  process.stdout.write(`${files.length} document(s), ${findings.length} error(s)\n`);
  return findings.length ? 1 : 0;
}

if (require.main === module) {
  process.exitCode = main(process.argv.slice(2), ROOT);
}

module.exports = { anchors, slug, checkRepository, main };
