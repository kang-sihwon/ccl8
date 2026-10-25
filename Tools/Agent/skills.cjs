#!/usr/bin/env node
'use strict';

// Checks the repository skills and keeps the Claude copy in step with the canonical .agents tree.
// Usage: node Tools/Agent/skills.cjs check | sync [--write]

const fs = require('node:fs');
const path = require('node:path');

const CANONICAL_TREE = '.agents/skills';
const MIRROR_TREES = ['.claude/skills'];
const MAX_DESCRIPTION_LENGTH = 1024;
const MAX_SKILL_LINES = 500;
const KEBAB_CASE = /^[a-z0-9]+(-[a-z0-9]+)*$/;
// Agent Skills keys plus the Claude Code extensions this project may use.
const KNOWN_KEYS = new Set(['name', 'description', 'license', 'compatibility', 'metadata', 'allowed-tools',
  'disable-model-invocation', 'user-invocable', 'argument-hint', 'when_to_use', 'model', 'context', 'agent', 'paths']);
const TRIGGER = /\buse (this )?(when|whenever|for|before|after|during)\b|사용(한다|하세요|할 때)|쓴다|쓸 때/i;
const NEGATED_TRIGGER = /\b(do not|don't|never) use (this )?(when|whenever|for|before|after|during)\b[^.]*\.?/gi;
const SKILL_REF = /`([a-z][a-z0-9]*(?:-[a-z0-9]+)+)`/g;
const CODE_PATH_REF = /`((?:\.\.?\/|references\/|examples\/|agents\/|features\/)[^`\s]*(?:\.\w+|\/))`/g;
const LINK_REF = /\]\(([^)\s#]+)(?:#[^)]*)?\)/g;
const INLINE_CODE = /`[^`]*`/g;

function stripFences(text) {
  let fence = null;
  return text.split(/\r?\n/).map(line => {
    const marker = line.match(/^ {0,3}(`{3,}|~{3,})/);
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

function parseFrontmatter(text) {
  const match = text.match(/^---\r?\n([\s\S]*?)\r?\n---\r?\n/);
  if (!match) return null;
  const fields = {};
  let key = null;
  for (const line of match[1].split(/\r?\n/)) {
    const top = line.match(/^([A-Za-z0-9_-]+)\s*:\s*(.*)$/);
    if (top) {
      key = top[1];
      const value = top[2].trim();
      fields[key] = /^[>|][-+]?$/.test(value) ? '' : value.replace(/^(['"])(.*)\1$/, '$2');
    } else if (key && /^\s+\S/.test(line) && !/^\s+[A-Za-z0-9_-]+\s*:/.test(line)) {
      fields[key] = `${fields[key]} ${line.trim()}`.trim();
    }
  }
  return fields;
}

function listFiles(root) {
  const files = [];
  const walk = dir => {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      const full = path.join(dir, entry.name);
      if (entry.isDirectory()) walk(full);
      else files.push(path.relative(root, full).split(path.sep).join('/'));
    }
  };
  if (fs.existsSync(root)) walk(root);
  return files.sort();
}

function listSkills(tree) {
  if (!fs.existsSync(tree)) return [];
  return fs.readdirSync(tree, { withFileTypes: true }).filter(entry => entry.isDirectory()).map(entry => entry.name).sort();
}

function lintSkill(skillDir, knownSkills) {
  const findings = [];
  const report = (level, message) => findings.push({ level, file: path.join(skillDir, 'SKILL.md'), message });
  const name = path.basename(skillDir);
  const file = path.join(skillDir, 'SKILL.md');
  if (!fs.existsSync(file)) {
    report('error', 'SKILL.md is missing');
    return findings;
  }
  const text = fs.readFileSync(file, 'utf8');
  const fields = parseFrontmatter(text);
  if (!fields) {
    report('error', 'frontmatter block is missing');
    return findings;
  }

  if (!KEBAB_CASE.test(name)) report('error', `directory name "${name}" is not lowercase-hyphenated`);
  if (fields.name !== name) report('error', `frontmatter name "${fields.name ?? ''}" does not match directory "${name}"`);
  const description = fields.description ?? '';
  if (!description) report('error', 'description is missing');
  if (description.length > MAX_DESCRIPTION_LENGTH) report('error', `description has ${description.length} characters, limit ${MAX_DESCRIPTION_LENGTH}`);
  if (description && !TRIGGER.test(description.replace(NEGATED_TRIGGER, ''))) report('error', 'description does not say when to use the skill');
  for (const key of Object.keys(fields)) {
    if (!KNOWN_KEYS.has(key)) report('warn', `unknown frontmatter key "${key}"`);
  }

  const lines = stripFences(text);
  if (lines.length > MAX_SKILL_LINES) report('warn', `SKILL.md has ${lines.length} lines, budget ${MAX_SKILL_LINES}`);
  const docs = [file, ...listFiles(skillDir).filter(rel => rel.endsWith('.md') && rel !== 'SKILL.md').map(rel => path.join(skillDir, rel))];
  for (const doc of docs) {
    const docLines = doc === file ? lines : stripFences(fs.readFileSync(doc, 'utf8'));
    docLines.forEach((line, index) => {
      const where = `${path.relative(skillDir, doc).split(path.sep).join('/')}:${index + 1}`;
      const namedSkills = /skill|스킬/i.test(line) ? [...line.matchAll(SKILL_REF)].map(match => match[1]).filter(ref => !KNOWN_KEYS.has(ref)) : [];
      const refs = [...line.matchAll(CODE_PATH_REF), ...line.replace(INLINE_CODE, '').matchAll(LINK_REF)].map(match => match[1]);
      for (const ref of refs) {
        // Placeholders such as (URL) and (url) have neither a dot nor a slash.
        if (/^[a-z]+:/i.test(ref) || /[[\]<>*{}]/.test(ref) || !/[./]/.test(ref)) continue;
        // Skill bodies name paths relative to the skill folder, supporting files link relative to themselves,
        // and a path next to another skill's name belongs to that skill.
        const bases = [path.dirname(doc), skillDir, ...namedSkills.map(skill => path.join(path.dirname(skillDir), skill))];
        if (!bases.some(base => fs.existsSync(path.resolve(base, decodeURIComponent(ref))))) {
          findings.push({ level: 'error', file: doc, message: `${where} references missing path "${ref}"` });
        }
      }
      for (const ref of namedSkills) {
        if (!knownSkills.has(ref)) {
          findings.push({ level: 'warn', file: doc, message: `${where} names unknown skill "${ref}"` });
        }
      }
    });
  }
  return findings;
}

function normalized(file) {
  return fs.readFileSync(file).toString('latin1').replace(/\r\n/g, '\n');
}

function compareTrees(root, canonicalTree, mirrorTree) {
  const findings = [];
  const canonical = path.join(root, canonicalTree);
  const mirror = path.join(root, mirrorTree);
  const left = new Set(listFiles(canonical));
  const right = new Set(listFiles(mirror));
  const canonicalSkills = new Set(listSkills(canonical));
  const add = (file, message, sync) => findings.push({ level: 'error', file, message, sync, canonical, mirror });
  for (const rel of left) {
    if (!right.has(rel)) add(path.join(mirror, rel), `missing, present in ${canonicalTree}`, 'copy');
    else if (normalized(path.join(canonical, rel)) !== normalized(path.join(mirror, rel))) add(path.join(mirror, rel), `differs from ${canonicalTree}`, 'copy');
  }
  for (const skill of listSkills(mirror).filter(skill => !canonicalSkills.has(skill))) {
    add(path.join(mirror, skill), `skill exists only here, not in ${canonicalTree}`, 'orphan');
  }
  for (const rel of right) {
    if (!left.has(rel) && canonicalSkills.has(rel.split('/')[0])) add(path.join(mirror, rel), `not in ${canonicalTree}`, 'orphan');
  }
  return findings;
}

function checkRepository(root) {
  const trees = [CANONICAL_TREE, ...MIRROR_TREES];
  const knownSkills = new Set(trees.flatMap(tree => listSkills(path.join(root, tree))));
  const findings = [];
  // Mirror copies are covered by the tree comparison, so only skills absent from the canonical tree are linted there.
  const canonicalSkills = new Set(listSkills(path.join(root, CANONICAL_TREE)));
  for (const tree of trees) {
    for (const skill of listSkills(path.join(root, tree))) {
      if (tree === CANONICAL_TREE || !canonicalSkills.has(skill)) findings.push(...lintSkill(path.join(root, tree, skill), knownSkills));
    }
  }
  for (const mirror of MIRROR_TREES) findings.push(...compareTrees(root, CANONICAL_TREE, mirror));
  return findings;
}

function main(argv, root) {
  const [mode, ...flags] = argv;
  const findings = checkRepository(root);
  if (mode === 'check') {
    for (const { level, file, message } of findings) {
      process.stdout.write(`${level.toUpperCase()} ${path.relative(root, file).split(path.sep).join('/')}: ${message}\n`);
    }
    const errors = findings.filter(finding => finding.level === 'error').length;
    process.stdout.write(`${errors} error(s), ${findings.length - errors} warning(s)\n`);
    return errors ? 1 : 0;
  }
  if (mode === 'sync') {
    // Copies canonical files over the mirror only with --write; orphans are reported, never deleted.
    const write = flags.includes('--write');
    for (const finding of findings.filter(item => item.sync)) {
      const rel = path.relative(finding.mirror, finding.file);
      const shown = path.relative(root, finding.file).split(path.sep).join('/');
      if (finding.sync === 'orphan') {
        process.stdout.write(`ORPHAN ${shown}: move it to ${CANONICAL_TREE} or delete it by hand\n`);
        continue;
      }
      process.stdout.write(`${write ? 'COPY' : 'WOULD COPY'} ${shown}\n`);
      if (write) {
        fs.mkdirSync(path.dirname(finding.file), { recursive: true });
        fs.copyFileSync(path.join(finding.canonical, rel), finding.file);
      }
    }
    return 0;
  }
  process.stderr.write('Usage: node Tools/Agent/skills.cjs check | sync [--write]\n');
  return 2;
}

if (require.main === module) {
  process.exitCode = main(process.argv.slice(2), path.resolve(__dirname, '..', '..'));
}

module.exports = { parseFrontmatter, lintSkill, compareTrees, checkRepository, main };
