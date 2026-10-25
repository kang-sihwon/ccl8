#!/usr/bin/env node
'use strict';

// Reports the mechanical C++ layout rules of Docs/Coding.md. It never edits sources.
// Usage: node Tools/Agent/cpp-rules.cjs check [--summary] [paths...]
//        node Tools/Agent/cpp-rules.cjs format [--base <rev>]   clang-format on lines changed since <rev> (default HEAD)

const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const ROOT = path.resolve(__dirname, '..', '..');
const CONTROL = /^(?:if(?:\s+constexpr)?|for|while|else\s+if(?:\s+constexpr)?)\s*\(/;
const ACCESS = /^(public|protected|private):\s*$/;
const BOOL_MEMBER = /^(?:mutable\s+|static\s+)?bool\s+b\w+\s*(?:=[^;]*|\{[^}]*\}|:\s*\d+)?\s*;\s*$/;
const RECORD_HEAD = /^(?:template\s*<.*>\s*)?(?:class|struct|union)\s+(?!.*;\s*$)/;

// Removes comments and string contents so braces and keywords inside them are ignored.
function codeLines(text) {
  let inBlock = false;
  return text.split(/\r?\n/).map(raw => {
    let line = raw;
    let out = '';
    while (line.length) {
      if (inBlock) {
        const end = line.indexOf('*/');
        if (end < 0) return out;
        line = line.slice(end + 2);
        inBlock = false;
        continue;
      }
      const match = line.match(/\/\*|\/\/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])+'/);
      if (!match) {
        out += line;
        break;
      }
      out += line.slice(0, match.index);
      if (match[0] === '//') break;
      if (match[0] === '/*') {
        inBlock = true;
        line = line.slice(match.index + 2);
        continue;
      }
      out += match[0][0] + match[0][0];
      line = line.slice(match.index + match[0].length);
    }
    return out;
  });
}

function parenEnd(text, start) {
  let depth = 0;
  for (let index = start; index < text.length; index++) {
    if (text[index] === '(') depth++;
    else if (text[index] === ')' && --depth === 0) return index;
  }
  return -1;
}

function checkSource(file, text) {
  const findings = [];
  const report = (rule, line, message) => findings.push({ rule, file, line, message });
  const isHeader = /\.(h|hpp)$/i.test(file);
  const raw = text.split(/\r?\n/);
  const code = codeLines(text);
  const nextCode = from => {
    for (let index = from; index < code.length; index++) {
      if (code[index].trim()) return index;
    }
    return -1;
  };

  const includes = [];
  const scopes = [];
  const conditions = [];
  let recordPending = false;
  for (let index = 0; index < code.length; index++) {
    const trimmed = code[index].trim();
    const number = index + 1;

    if (!raw[index].trim() && index > 0 && !raw[index - 1].trim()) report('blank-double', number, 'two blank lines in a row');
    if (trimmed.startsWith('#')) {
      // Directives are read from the raw line because string stripping blanks the include names.
      const directive = raw[index].trim().replace(/^#\s*/, '');
      const include = directive.match(/^include\s*[<"]([^>"]+)[>"]/);
      if (include) includes.push({ name: include[1], line: number });
      if (/^if/.test(directive)) {
        const name = (directive.match(/^if(?:def)?\s+(\w+)/) || [])[1] ?? '';
        if (name === 'WITH_EDITORONLY_DATA' && conditions.includes('WITH_EDITOR')) {
          report('editor-only-data-nested', number, 'WITH_EDITORONLY_DATA is nested inside WITH_EDITOR');
        }
        conditions.push(name);
      } else if (/^endif/.test(directive)) conditions.pop();
      continue;
    }
    if (!trimmed) continue;

    if (isHeader && conditions[conditions.length - 1] === 'WITH_EDITOR' && /^UPROPERTY\s*\(/.test(trimmed)) {
      report('editor-uproperty', number, 'UPROPERTY inside WITH_EDITOR; use WITH_EDITORONLY_DATA');
    }
    const top = scopes[scopes.length - 1];
    if (isHeader && top === 'record' && BOOL_MEMBER.test(trimmed) && !/\bconstexpr\b/.test(trimmed)) {
      report('bool-member', number, 'header boolean member must be uint8 (decision 4)');
    }
    if (isHeader && ACCESS.test(trimmed) && index + 1 < raw.length && !raw[index + 1].trim()) {
      report('access-blank', number + 1, 'blank line right after an access specifier');
    }
    if (isHeader && /^GENERATED_(?:U\w+_)?BODY\(\)/.test(trimmed)) {
      // Region comments count as the next declaration, so the gap is measured on raw lines.
      let next = index + 1;
      while (next < raw.length && !raw[next].trim()) next++;
      const gap = next - index - 1;
      const closes = next < raw.length && /^}/.test(raw[next].trim());
      if (closes ? gap !== 0 : gap !== 1) report('generated-body-blank', number, `GENERATED_BODY() must be followed by ${closes ? 'no' : 'one'} blank line`);
    }
    if (/^}/.test(trimmed) && index > 0 && !raw[index - 1].trim()) report('blank-before-close', number - 1, 'blank line right before a closing brace');
    if (trimmed === '{' && index + 1 < raw.length && !raw[index + 1].trim()) {
      report('blank-after-open', number + 1, 'blank line right after an opening brace');
    }

    // Decision 14: control bodies are braced blocks whose braces sit on their own lines.
    if (/^}\s*else\b/.test(trimmed)) report('else-line', number, 'else must start its own line after the closing brace');
    const control = trimmed.replace(/^}\s*/, '');
    if (CONTROL.test(control)) {
      let joined = control;
      let last = index;
      let close = parenEnd(joined, joined.indexOf('('));
      while (close < 0 && last + 1 < code.length) {
        joined += ` ${code[++last].trim()}`;
        close = parenEnd(joined, joined.indexOf('('));
      }
      const rest = close < 0 ? '' : joined.slice(close + 1).trim();
      if (rest.startsWith('{')) report('brace-own-line', last + 1, rest.endsWith('}') ? 'single-line block' : 'opening brace on the condition line');
      else if (rest && rest !== ';') report('braced-body', last + 1, 'body on the condition line without braces');
      else if (!rest) {
        const next = nextCode(last + 1);
        const body = next < 0 ? '{' : code[next].trim();
        if (body.startsWith('{') && body !== '{') report('brace-own-line', next + 1, 'single-line block');
        else if (body !== '{') report('braced-body', next + 1, 'control body without braces');
      }
    } else if (/^else\b/.test(control)) {
      const rest = control.slice(4).trim();
      if (rest.startsWith('{')) report('brace-own-line', number, rest.endsWith('}') ? 'single-line block' : 'opening brace on the else line');
      else if (rest) report('braced-body', number, 'else body without braces');
      else {
        const next = nextCode(index + 1);
        if (next >= 0 && code[next].trim() !== '{') report('braced-body', next + 1, 'else body without braces');
      }
    }
    if (!isHeader && scopes.every(scope => scope === 'namespace') && /\)\s*(?:const\s*)?(?:override\s*)?{.*}\s*$/.test(trimmed) && !/=/.test(trimmed.split('(')[0])) {
      report('cpp-one-line-function', number, 'function defined on one line in a .cpp file');
    }

    if (RECORD_HEAD.test(trimmed)) recordPending = true;
    else if (/^(?:namespace\b|enum\b)/.test(trimmed) && !trimmed.endsWith(';')) recordPending = trimmed.startsWith('namespace') ? 'namespace' : 'enum';
    for (const char of code[index]) {
      if (char === '{') {
        scopes.push(recordPending === true ? 'record' : recordPending || 'block');
        recordPending = false;
      } else if (char === '}') scopes.pop();
      else if (char === ';' && recordPending === true && !scopes.length) recordPending = false;
    }
  }

  if (isHeader) {
    const first = nextCode(0);
    if (first < 0 || !/^#\s*pragma\s+once\b/.test(code[first].trim())) report('pragma-once', first + 1, 'header must start with #pragma once');
    const generated = includes.findIndex(include => include.name.endsWith('.generated.h'));
    if (generated >= 0 && generated !== includes.length - 1) report('generated-include-last', includes[generated].line, '.generated.h must be the last include');
    const core = includes.findIndex(include => include.name === 'CoreMinimal.h');
    if (core > 0) report('coreminimal-first', includes[core].line, 'CoreMinimal.h must be the first include');
  } else if (includes.length) {
    const own = `${path.basename(file).replace(/\.cpp$/i, '')}.h`;
    const ownIndex = includes.findIndex(include => path.basename(include.name) === own);
    if (ownIndex > 0) report('own-header-first', includes[ownIndex].line, `${own} must be the first include`);
  }
  return findings;
}

function sourceFiles(targets) {
  const files = [];
  const walk = target => {
    const stat = fs.statSync(target);
    if (stat.isDirectory()) for (const entry of fs.readdirSync(target)) walk(path.join(target, entry));
    else if (/\.(h|hpp|cpp)$/i.test(target)) files.push(target);
  };
  for (const target of targets) walk(path.resolve(ROOT, target));
  return files.sort();
}

function clangFormat() {
  if (process.env.CLANG_FORMAT) return process.env.CLANG_FORMAT;
  try {
    return execFileSync('where', ['clang-format'], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).split(/\r?\n/)[0];
  } catch {
    const vswhere = path.join(process.env['ProgramFiles(x86)'] ?? '', 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
    if (!fs.existsSync(vswhere)) return null;
    const found = execFileSync(vswhere, ['-latest', '-find', 'VC\\Tools\\Llvm\\x64\\bin\\clang-format.exe'], { encoding: 'utf8' }).trim();
    return found.split(/\r?\n/)[0] || null;
  }
}

function changedRanges(base) {
  const diff = execFileSync('git', ['diff', '-U0', '--no-color', base, '--', '*.h', '*.hpp', '*.cpp'], { cwd: ROOT, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  const ranges = new Map();
  let file = null;
  for (const line of diff.split('\n')) {
    const target = line.match(/^\+\+\+ b\/(.+)$/);
    if (target) file = target[1];
    else if (line.startsWith('+++ /dev/null')) file = null;
    const hunk = line.match(/^@@ -\S+ \+(\d+)(?:,(\d+))? @@/);
    if (hunk && file && hunk[2] !== '0') {
      const start = Number(hunk[1]);
      if (!ranges.has(file)) ranges.set(file, []);
      ranges.get(file).push(`${start}:${start + Number(hunk[2] ?? 1) - 1}`);
    }
  }
  return ranges;
}

function main(argv) {
  const [mode, ...rest] = argv;
  if (mode === 'check') {
    const summary = rest.includes('--summary');
    const targets = rest.filter(arg => !arg.startsWith('--'));
    const findings = sourceFiles(targets.length ? targets : ['Source']).flatMap(file => checkSource(file, fs.readFileSync(file, 'utf8')));
    const counts = new Map();
    for (const finding of findings) {
      counts.set(finding.rule, (counts.get(finding.rule) ?? 0) + 1);
      if (!summary) process.stdout.write(`${finding.rule} ${path.relative(ROOT, finding.file).split(path.sep).join('/')}:${finding.line} ${finding.message}\n`);
    }
    for (const [rule, count] of [...counts].sort((a, b) => b[1] - a[1])) process.stdout.write(`${String(count).padStart(6)} ${rule}\n`);
    process.stdout.write(`${findings.length} finding(s) in ${new Set(findings.map(finding => finding.file)).size} file(s)\n`);
    return findings.length ? 1 : 0;
  }
  if (mode === 'format') {
    const baseIndex = rest.indexOf('--base');
    const base = baseIndex >= 0 ? rest[baseIndex + 1] : 'HEAD';
    const tool = clangFormat();
    if (!tool) throw new Error('clang-format not found; set CLANG_FORMAT or install the Visual Studio LLVM tools.');
    let failed = 0;
    for (const [file, ranges] of changedRanges(base)) {
      const args = ['--dry-run', '--Werror', '--style=file', ...ranges.map(range => `--lines=${range}`), path.join(ROOT, file)];
      try {
        execFileSync(tool, args, { cwd: ROOT, encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] });
      } catch (error) {
        failed++;
        process.stdout.write(error.stderr || error.stdout || `${file}: clang-format failed\n`);
      }
    }
    return failed ? 1 : 0;
  }
  process.stderr.write('Usage: node Tools/Agent/cpp-rules.cjs check [--summary] [paths...] | format [--base <rev>]\n');
  return 2;
}

if (require.main === module) {
  try {
    process.exitCode = main(process.argv.slice(2));
  } catch (error) {
    process.stderr.write(`${error.message}\n`);
    process.exitCode = 2;
  }
}

module.exports = { checkSource, codeLines, clangFormat };
