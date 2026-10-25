#!/usr/bin/env node
'use strict';

// Claude Code PreToolUse hook: destructive Git and delete commands need the user's confirmation first.
// Reads the hook payload on stdin and answers "ask" with the matched reason; anything else passes silently.

const RULES = [
  [/\bgit\s+reset\b[^\n;&|]*--(hard|merge|keep)\b/, 'git reset discards working tree changes'],
  [/\bgit\s+clean\b[^\n;&|]*\s-[a-z]*f/, 'git clean deletes untracked files'],
  [/\bgit\s+checkout\b[^\n;&|]*(\s--\s|\s-f\b|\s--force\b|\s\.(\s|$))/, 'git checkout overwrites working tree files'],
  [/\bgit\s+restore\b(?![^\n;&|]*--staged\b(?![^\n;&|]*--worktree))/, 'git restore overwrites working tree files'],
  [/\bgit\s+push\b[^\n;&|]*(\s-f\b|--force|--mirror|--delete|\s-d\b|\s\+\S|\s:\S)/, 'git push rewrites or deletes remote refs'],
  [/\bgit\s+branch\b[^\n;&|]*(\s-D\b|--delete\b[^\n;&|]*--force|\s-d\b[^\n;&|]*\s-f\b)/, 'git branch deletes a branch'],
  [/\bgit\s+tag\b[^\n;&|]*(\s-d\b|--delete\b)/, 'git tag deletes a tag'],
  [/\bgit\s+stash\s+(drop|clear)\b/, 'git stash drops saved changes'],
  [/\bgit\s+(filter-branch|filter-repo|update-ref\s+-d|reflog\s+expire)\b/, 'git rewrites or expires history'],
  [/\bgit\s+gc\b[^\n;&|]*--prune/, 'git gc prunes unreachable objects'],
  [/\bgit\s+rm\b/, 'git rm deletes files'],
  [/(^|[\s;&|(])(rm|rmdir|unlink|shred)\s/, 'shell command deletes files'],
  [/(^|[\s;&|(])(del|erase|rd)\s+\S/i, 'cmd command deletes files'],
  [/\b(Remove-Item|Clear-Content)\b|(^|[\s;&|(])ri\s/i, 'PowerShell command deletes files'],
  [/\bfind\b[^\n;&|]*(-delete\b|-exec\s+rm\b)/, 'find deletes files'],
];

// Global options such as -C <path> or -c key=value sit between git and its subcommand.
const normalize = command => command.replace(/\bgit((?:\s+(?:-C|-c)\s+\S+|\s+--[a-z-]+(?:=\S+)?)+)(?=\s)/g, 'git');

function decide(payload) {
  const command = payload?.tool_input?.command;
  if (typeof command !== 'string') return null;
  const text = normalize(command);
  const reasons = RULES.filter(([pattern]) => pattern.test(text)).map(([, reason]) => reason);
  if (!reasons.length) return null;
  return {
    hookSpecificOutput: {
      hookEventName: 'PreToolUse',
      permissionDecision: 'ask',
      permissionDecisionReason: `Confirmation required before running: ${[...new Set(reasons)].join('; ')}.`,
    },
  };
}

if (require.main === module) {
  let input = '';
  process.stdin.setEncoding('utf8');
  process.stdin.on('data', chunk => { input += chunk; });
  process.stdin.on('end', () => {
    let payload = null;
    try {
      payload = JSON.parse(input);
    } catch {
      return;
    }
    const decision = decide(payload);
    if (decision) process.stdout.write(`${JSON.stringify(decision)}\n`);
  });
}

module.exports = { decide };
