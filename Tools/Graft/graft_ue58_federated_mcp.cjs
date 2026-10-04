"use strict";

const fs = require("fs");
const path = require("path");
const readline = require("readline");
const { execFileSync, spawn, spawnSync } = require("child_process");
const { pathToFileURL } = require("url");

const SERVER_VERSION = "1.0.0";
const DEFAULT_LIMIT = 5;
const DEFAULT_MAX_SHARDS = 6;
const WORKER_TIMEOUT_MS = 300000;
const MAX_COMBINED_CHARS = 80000;

const CORE_SHARDS = [
  "Source_Runtime_Batched",
  "Source_Editor",
  "Source_Developer",
  "Source_Programs",
];

const GENERIC_QUERY_WORDS = new Set([
  "about", "after", "before", "build", "call", "calls", "code", "engine",
  "file", "find", "flow", "from", "function", "how", "implementation",
  "inside", "method", "source", "system", "the", "this", "trace", "unreal",
  "usage", "uses", "using", "what", "where", "with",
]);

function fail(message) {
  throw new Error(message);
}

function parseArguments(argv) {
  const options = {
    indexRoot: process.env.UE58_GRAFT_INDEX_ROOT || "",
    engineRoot: process.env.UE58_ENGINE_ROOT || "",
    probe: "",
    listShards: false,
    workerPayload: "",
  };

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    const next = () => {
      index += 1;
      if (index >= argv.length) {
        fail(`Missing value for ${arg}`);
      }
      return argv[index];
    };

    switch (arg) {
      case "--index-root":
        options.indexRoot = next();
        break;
      case "--engine-root":
        options.engineRoot = next();
        break;
      case "--probe":
        options.probe = next();
        break;
      case "--list-shards":
        options.listShards = true;
        break;
      case "--worker":
        options.workerPayload = next();
        break;
      default:
        fail(`Unknown argument: ${arg}`);
    }
  }

  return options;
}

function normalizeAbsoluteDirectory(value, label) {
  if (!value) {
    fail(`${label} is required.`);
  }
  const resolved = path.resolve(value);
  if (!fs.existsSync(resolved) || !fs.statSync(resolved).isDirectory()) {
    fail(`${label} was not found: ${resolved}`);
  }
  return resolved;
}

function discoverShards(indexRoot) {
  const shards = [];
  for (const entry of fs.readdirSync(indexRoot, { withFileTypes: true })) {
    if (!entry.isDirectory() || entry.name.startsWith(".")) {
      continue;
    }

    const indexDir = path.join(indexRoot, entry.name);
    const indexFile = path.join(indexDir, "INDEX.md");
    const wiringFile = path.join(indexDir, ".graph", "wiring.json");
    if (!fs.existsSync(indexFile) || !fs.existsSync(wiringFile)) {
      continue;
    }

    const indexText = fs.readFileSync(indexFile, "utf8");
    const cardMatch = indexText.match(/([0-9]+) per-file wiring cards/);
    shards.push({
      name: entry.name,
      indexDir,
      cards: cardMatch ? Number(cardMatch[1]) : 0,
      wiringBytes: fs.statSync(wiringFile).size,
    });
  }

  const priority = new Map(CORE_SHARDS.map((name, index) => [name, index]));
  shards.sort((left, right) => {
    const leftPriority = priority.has(left.name) ? priority.get(left.name) : 100;
    const rightPriority = priority.has(right.name) ? priority.get(right.name) : 100;
    return leftPriority - rightPriority || left.name.localeCompare(right.name);
  });

  if (shards.length === 0) {
    fail(`No Graft shard was found under ${indexRoot}`);
  }
  return shards;
}

function findGraftPackageRoot() {
  const explicit = process.env.GRAFT_PACKAGE_ROOT;
  if (explicit && fs.existsSync(path.join(explicit, "dist", "engine.js"))) {
    return path.resolve(explicit);
  }

  const candidates = [];
  if (process.env.APPDATA) {
    candidates.push(path.join(process.env.APPDATA, "npm", "node_modules", "@nanonets", "graft"));
  }

  try {
    const npmCommand = process.platform === "win32" ? "npm.cmd" : "npm";
    const npmRoot = execFileSync(npmCommand, ["root", "-g"], {
      encoding: "utf8",
      windowsHide: true,
      stdio: ["ignore", "pipe", "ignore"],
    }).trim();
    if (npmRoot) {
      candidates.push(path.join(npmRoot, "@nanonets", "graft"));
    }
  } catch {
    // The explicit and APPDATA candidates still provide a useful fallback.
  }

  for (const candidate of candidates) {
    if (fs.existsSync(path.join(candidate, "dist", "engine.js"))) {
      return candidate;
    }
  }
  fail("The global @nanonets/graft package was not found. Run npm install -g @nanonets/graft@0.18.0.");
}

async function importGraftModule(packageRoot, relativePath) {
  const modulePath = path.join(packageRoot, "dist", relativePath);
  return import(pathToFileURL(modulePath).href);
}

async function loadGraftModules() {
  const packageRoot = findGraftPackageRoot();
  const [
    engineModule,
    askModule,
    graphLoadModule,
    graphTraverseModule,
    graphTraverseCliModule,
    grepModule,
    grepCliModule,
    mapModule,
    graphCheckModule,
    savingsModule,
    mcpToolsModule,
    mcpInstructionsModule,
  ] = await Promise.all([
    importGraftModule(packageRoot, "engine.js"),
    importGraftModule(packageRoot, path.join("ask", "ask.js")),
    importGraftModule(packageRoot, path.join("graph", "load.js")),
    importGraftModule(packageRoot, path.join("graph", "traverse.js")),
    importGraftModule(packageRoot, path.join("graph", "traverse-cli.js")),
    importGraftModule(packageRoot, path.join("search", "grep.js")),
    importGraftModule(packageRoot, path.join("search", "grep-cli.js")),
    importGraftModule(packageRoot, path.join("graph", "map.js")),
    importGraftModule(packageRoot, path.join("graph", "check.js")),
    importGraftModule(packageRoot, path.join("context", "savings.js")),
    importGraftModule(packageRoot, path.join("mcp", "tools.js")),
    importGraftModule(packageRoot, path.join("mcp", "instructions.js")),
  ]);

  return {
    packageRoot,
    ...engineModule,
    ...askModule,
    ...graphLoadModule,
    ...graphTraverseModule,
    ...graphTraverseCliModule,
    ...grepModule,
    ...grepCliModule,
    ...mapModule,
    ...graphCheckModule,
    ...savingsModule,
    TOOLS: mcpToolsModule.TOOLS,
    mcpInstructions: mcpInstructionsModule.mcpInstructions,
  };
}

function normalizeRepoPath(value) {
  return String(value || "")
    .replace(/^['"]|['"]$/g, "")
    .replace(/\\/g, "/")
    .replace(/^\.\//, "")
    .replace(/^\/+/, "");
}

function cardPathFor(repoRelativeFile) {
  const normalized = normalizeRepoPath(repoRelativeFile).replace(/:L\d+(?:-L\d+)?$/, "");
  if (!normalized.includes(".")) {
    return "";
  }
  return normalized.replace(/\.[^/.]+$/, ".md");
}

function resolveShardHint(shards, value) {
  if (!value) {
    return null;
  }
  const normalized = String(value).trim().toLowerCase();
  const exact = shards.find((shard) => shard.name.toLowerCase() === normalized);
  if (exact) {
    return exact;
  }
  const matches = shards.filter((shard) => shard.name.toLowerCase().includes(normalized));
  return matches.length === 1 ? matches[0] : null;
}

function splitWorkspaceIn(shards, inValue) {
  const normalized = normalizeRepoPath(inValue);
  if (!normalized) {
    return { shard: null, childIn: "" };
  }
  const slash = normalized.indexOf("/");
  const first = slash === -1 ? normalized : normalized.slice(0, slash);
  const shard = resolveShardHint(shards, first);
  if (!shard || shard.name.toLowerCase() !== first.toLowerCase()) {
    return { shard: null, childIn: normalized };
  }
  return {
    shard,
    childIn: slash === -1 ? "" : normalized.slice(slash + 1),
  };
}

function categoryShards(shards, value) {
  const normalized = normalizeRepoPath(value).toLowerCase();
  if (normalized.startsWith("source/runtime/") || normalized === "source/runtime") {
    return shards.filter((shard) => shard.name === "Source_Runtime_Batched");
  }
  if (normalized.startsWith("source/editor/") || normalized === "source/editor") {
    return shards.filter((shard) => shard.name === "Source_Editor");
  }
  if (normalized.startsWith("source/developer/") || normalized === "source/developer") {
    return shards.filter((shard) => shard.name === "Source_Developer");
  }
  if (normalized.startsWith("source/programs/") || normalized === "source/programs") {
    return shards.filter((shard) => shard.name === "Source_Programs");
  }
  if (normalized.startsWith("source/thirdparty/") || normalized === "source/thirdparty") {
    return shards.filter((shard) => shard.name.startsWith("Source_ThirdParty_"));
  }
  if (normalized.startsWith("plugins/") || normalized === "plugins") {
    return shards.filter((shard) => shard.name.startsWith("Plugins_Batched_"));
  }
  return [];
}

function cardExists(shard, repoRelativeFile) {
  const cardPath = cardPathFor(repoRelativeFile);
  return Boolean(cardPath) && fs.existsSync(path.join(shard.indexDir, ...cardPath.split("/")));
}

function shardsForFile(shards, repoRelativeFile) {
  const categories = categoryShards(shards, repoRelativeFile);
  const candidates = categories.length ? categories : shards;
  return candidates.filter((shard) => cardExists(shard, repoRelativeFile));
}

function extractRoutingTokens(query) {
  const raw = String(query || "").match(/[A-Za-z_][A-Za-z0-9_:./<>-]{2,}/g) || [];
  const unique = [];
  const seen = new Set();
  for (const original of raw) {
    const token = original.replace(/[<>.,;(){}\[\]]+$/g, "");
    const lower = token.toLowerCase();
    if (token.length < 3 || GENERIC_QUERY_WORDS.has(lower) || seen.has(lower)) {
      continue;
    }
    seen.add(lower);
    unique.push(token);
  }
  unique.sort((left, right) => {
    const leftIdentifier = /[A-Z_]|::/.test(left) ? 1 : 0;
    const rightIdentifier = /[A-Z_]|::/.test(right) ? 1 : 0;
    return rightIdentifier - leftIdentifier || right.length - left.length;
  });
  return unique.slice(0, 3);
}

function runRipgrep(args, options = {}) {
  const result = spawnSync("rg", args, {
    encoding: "utf8",
    windowsHide: true,
    timeout: options.timeout || 20000,
    maxBuffer: options.maxBuffer || 64 * 1024 * 1024,
  });
  if (result.error) {
    return { ok: false, output: "", error: result.error.message };
  }
  if (result.status !== 0 && result.status !== 1) {
    return { ok: false, output: result.stdout || "", error: result.stderr || `rg exited ${result.status}` };
  }
  return { ok: true, output: result.stdout || "", error: "" };
}

function routeByCards(indexRoot, shards, query) {
  const scores = new Map();
  const tokens = extractRoutingTokens(query);
  tokens.forEach((token, tokenIndex) => {
    const result = runRipgrep([
      "--files-with-matches",
      "--fixed-strings",
      "--ignore-case",
      "--max-count", "1",
      "--glob", "*.md",
      "--glob", "!INDEX.md",
      token,
      indexRoot,
    ]);
    if (!result.ok) {
      return;
    }
    for (const line of result.output.split(/\r?\n/)) {
      if (!line) {
        continue;
      }
      const relative = path.relative(indexRoot, line);
      const shardName = relative.split(path.sep)[0];
      if (!shards.some((shard) => shard.name === shardName)) {
        continue;
      }
      const weight = (tokens.length - tokenIndex) * 100;
      scores.set(shardName, (scores.get(shardName) || 0) + weight + 1);
    }
  });

  return [...scores.entries()]
    .sort((left, right) => right[1] - left[1] || left[0].localeCompare(right[0]))
    .map(([name]) => shards.find((shard) => shard.name === name));
}

function sourceFilesMatching(engineRoot, pattern, args) {
  const searchRoots = ["Source", "Plugins"]
    .map((name) => path.join(engineRoot, name))
    .filter((candidate) => fs.existsSync(candidate));
  const rgArgs = ["--files-with-matches"];
  if (args.ignore_case === true) {
    rgArgs.push("--ignore-case");
  }
  if (args.fixed === true) {
    rgArgs.push("--fixed-strings");
  }
  rgArgs.push(pattern, ...searchRoots);
  const result = runRipgrep(rgArgs, { timeout: 60000, maxBuffer: 128 * 1024 * 1024 });
  if (!result.ok) {
    return null;
  }
  return result.output
    .split(/\r?\n/)
    .filter(Boolean)
    .map((file) => normalizeRepoPath(path.relative(engineRoot, file)));
}

function uniqueShards(items) {
  const seen = new Set();
  return items.filter((item) => {
    if (!item || seen.has(item.name)) {
      return false;
    }
    seen.add(item.name);
    return true;
  });
}

function chooseShards(context, text, args, options = {}) {
  const explicit = resolveShardHint(context.shards, args.shard);
  if (args.shard && !explicit) {
    fail(`Unknown or ambiguous shard "${args.shard}".`);
  }
  if (explicit) {
    return { selected: [explicit], childIn: normalizeRepoPath(args.in), matched: [explicit] };
  }

  const workspaceIn = splitWorkspaceIn(context.shards, args.in);
  if (workspaceIn.shard) {
    return { selected: [workspaceIn.shard], childIn: workspaceIn.childIn, matched: [workspaceIn.shard] };
  }

  if (options.file) {
    const fileMatches = shardsForFile(context.shards, options.file);
    return { selected: fileMatches, childIn: "", matched: fileMatches };
  }

  const categoryMatches = categoryShards(context.shards, args.in || text);
  const cardMatches = routeByCards(context.indexRoot, context.shards, text);
  const matched = uniqueShards([...cardMatches, ...categoryMatches]);
  const fallback = CORE_SHARDS
    .map((name) => context.shards.find((shard) => shard.name === name))
    .filter(Boolean);
  const maxShards = Number.isFinite(args.max_shards)
    ? Math.max(1, Math.min(26, Math.floor(args.max_shards)))
    : DEFAULT_MAX_SHARDS;
  const selected = (matched.length ? matched : fallback).slice(0, maxShards);
  return {
    selected,
    childIn: normalizeRepoPath(args.in),
    matched,
    omitted: Math.max(0, (matched.length ? matched.length : fallback.length) - selected.length),
  };
}

function encodeWorkerPayload(payload) {
  return Buffer.from(JSON.stringify(payload), "utf8").toString("base64url");
}

function runWorker(context, shard, tool, args) {
  const payload = encodeWorkerPayload({
    engineRoot: context.engineRoot,
    indexDir: shard.indexDir,
    shard: shard.name,
    tool,
    args,
  });

  return new Promise((resolve) => {
    const child = spawn(process.execPath, [__filename, "--worker", payload], {
      windowsHide: true,
      env: { ...process.env, GRAFT_PACKAGE_ROOT: context.graftPackageRoot },
      stdio: ["ignore", "pipe", "pipe"],
    });
    let stdout = "";
    let stderr = "";
    let settled = false;
    const timer = setTimeout(() => {
      child.kill();
      if (!settled) {
        settled = true;
        resolve({ ok: false, shard: shard.name, error: `worker timed out after ${WORKER_TIMEOUT_MS} ms` });
      }
    }, WORKER_TIMEOUT_MS);

    child.stdout.on("data", (chunk) => {
      stdout += chunk.toString("utf8");
    });
    child.stderr.on("data", (chunk) => {
      stderr += chunk.toString("utf8");
    });
    child.on("error", (error) => {
      clearTimeout(timer);
      if (!settled) {
        settled = true;
        resolve({ ok: false, shard: shard.name, error: error.message });
      }
    });
    child.on("close", (code) => {
      clearTimeout(timer);
      if (settled) {
        return;
      }
      settled = true;
      if (code !== 0) {
        resolve({ ok: false, shard: shard.name, error: stderr.trim() || `worker exited ${code}` });
        return;
      }
      try {
        resolve(JSON.parse(stdout));
      } catch (error) {
        resolve({ ok: false, shard: shard.name, error: `invalid worker output: ${error.message}` });
      }
    });
  });
}

async function mapConcurrent(items, concurrency, callback) {
  const results = new Array(items.length);
  let next = 0;
  const workers = Array.from({ length: Math.min(concurrency, items.length) }, async () => {
    for (;;) {
      const current = next;
      next += 1;
      if (current >= items.length) {
        return;
      }
      results[current] = await callback(items[current], current);
    }
  });
  await Promise.all(workers);
  return results;
}

function truncateCombined(text) {
  if (text.length <= MAX_COMBINED_CHARS) {
    return text;
  }
  return `${text.slice(0, MAX_COMBINED_CHARS)}\n\n[wrapper truncated ${text.length - MAX_COMBINED_CHARS} characters]`;
}

function shardSummary(context) {
  const totalCards = context.shards.reduce((sum, shard) => sum + shard.cards, 0);
  const totalBytes = context.shards.reduce((sum, shard) => sum + shard.wiringBytes, 0);
  const lines = [
    `UE 5.8 federated Graft: ${context.shards.length} shards, ${totalCards.toLocaleString("en-US")} cards, ${(totalBytes / 1073741824).toFixed(2)} GiB wiring`,
    "",
  ];
  for (const shard of context.shards) {
    lines.push(`- ${shard.name}: ${shard.cards.toLocaleString("en-US")} cards, ${(shard.wiringBytes / 1048576).toFixed(1)} MiB`);
  }
  lines.push("", "Pass shard:\"<name>\" to inspect one shard. Query tools route by symbol and file evidence before loading graphs.");
  return lines.join("\n");
}

function routingNote(route) {
  const names = route.selected.map((shard) => shard.name).join(", ");
  const omitted = route.omitted ? `; ${route.omitted} additional matched shard(s) omitted — raise max_shards or pass shard` : "";
  return `router searched: ${names || "none"}${omitted}`;
}

async function handleFindCode(context, args, modules) {
  const query = String(args.query || "").trim();
  if (!query) {
    return { text: "graft_find_code requires a query", isError: true };
  }
  const route = chooseShards(context, query, args);
  if (route.selected.length === 0) {
    return { text: `No shard could be routed for "${query}". Pass shard explicitly.`, isError: true };
  }

  const limit = Number.isFinite(args.limit) ? Math.max(1, Math.floor(args.limit)) : DEFAULT_LIMIT;
  const workerArgs = {
    query,
    limit: Math.max(limit * 3, 12),
    full: args.full === true,
    in: route.childIn || "",
  };
  const workerResults = await mapConcurrent(route.selected, 2, (shard) => runWorker(context, shard, "find_code", workerArgs));
  const successful = workerResults.filter((result) => result && result.ok && result.result);
  const queues = successful.map((result) => result.result.hits.map((hit) => ({
    ...hit,
    scope: hit.scope ? `${result.shard}/${hit.scope}` : result.shard,
  })));
  const hits = [];
  for (let round = 0; hits.length < limit; round += 1) {
    let emitted = false;
    for (const queue of queues) {
      if (queue[round]) {
        hits.push(queue[round]);
        emitted = true;
        if (hits.length >= limit) {
          break;
        }
      }
    }
    if (!emitted) {
      break;
    }
  }

  const saved = successful.reduce((total, result) => {
    const value = result.result.saved;
    if (value) {
      total.files += value.files || 0;
      total.baselineChars += value.baselineChars || 0;
    }
    return total;
  }, { files: 0, baselineChars: 0 });

  const errors = workerResults.filter((result) => !result || !result.ok);
  const notes = [routingNote(route)];
  if (errors.length) {
    notes.push(`worker errors: ${errors.map((result) => `${result.shard}: ${result.error}`).join("; ")}`);
  }
  if (hits.length === 0) {
    notes.push("no matching nodes in routed shards — pass shard or use graft_find_all with a literal identifier");
  }

  const result = {
    query,
    mode: hits.length ? "lexical" : "empty",
    hits,
    note: notes.join("\n"),
  };
  if (saved.baselineChars > 0) {
    result.saved = saved;
  }
  return { text: modules.formatAsk(result), isError: false };
}

async function handleFileApi(context, args) {
  let file = normalizeRepoPath(args.file);
  if (!file) {
    return { text: "graft_file_api requires a file", isError: true };
  }

  const workspaceIn = splitWorkspaceIn(context.shards, file);
  let candidates;
  if (args.shard) {
    const explicit = resolveShardHint(context.shards, args.shard);
    if (!explicit) {
      return { text: `Unknown or ambiguous shard "${args.shard}".`, isError: true };
    }
    candidates = [explicit];
  } else if (workspaceIn.shard) {
    candidates = [workspaceIn.shard];
    file = workspaceIn.childIn;
  } else {
    candidates = shardsForFile(context.shards, file);
  }

  if (!candidates.length) {
    return {
      text: `No shard owns ${file}. Pass an engine-relative path such as Source/Runtime/... or Plugins/... and optionally shard.`,
      isError: true,
    };
  }
  const results = await mapConcurrent(candidates, 2, (shard) => runWorker(context, shard, "file_api", { file }));
  const blocks = results.filter((result) => result && result.ok).map((result) => `## [${result.shard}]\n${result.text}`);
  const errors = results.filter((result) => !result || !result.ok);
  if (errors.length) {
    blocks.push(`worker errors: ${errors.map((result) => `${result.shard}: ${result.error}`).join("; ")}`);
  }
  return { text: truncateCombined(blocks.join("\n\n")), isError: blocks.length === 0 };
}

async function handleTraceCalls(context, args) {
  const symbol = String(args.symbol || args.file || "").trim();
  if (!symbol) {
    return { text: "graft_trace_calls requires a symbol", isError: true };
  }
  const route = chooseShards(context, symbol, args);
  const results = await mapConcurrent(route.selected, 2, (shard) => runWorker(context, shard, "trace_calls", {
    symbol,
    direction: args.direction === "out" ? "out" : "in",
    depth: args.depth,
    in: route.childIn || "",
  }));
  const blocks = results
    .filter((result) => result && result.ok && result.found)
    .map((result) => `## [${result.shard}]\n${result.text}`);
  blocks.push(routingNote(route));
  const errors = results.filter((result) => !result || !result.ok);
  if (errors.length) {
    blocks.push(`worker errors: ${errors.map((result) => `${result.shard}: ${result.error}`).join("; ")}`);
  }
  return { text: truncateCombined(blocks.join("\n\n")), isError: !results.some((result) => result && result.ok && result.found) };
}

async function handleFindAll(context, args) {
  const pattern = String(args.pattern || "");
  if (!pattern) {
    return { text: "graft_find_all requires a pattern", isError: true };
  }

  let selected;
  if (args.shard) {
    const explicit = resolveShardHint(context.shards, args.shard);
    if (!explicit) {
      return { text: `Unknown or ambiguous shard "${args.shard}".`, isError: true };
    }
    selected = [explicit];
  } else {
    const files = sourceFilesMatching(context.engineRoot, pattern, args);
    if (files === null) {
      selected = chooseShards(context, pattern, { ...args, max_shards: 26 }).selected;
    } else {
      selected = uniqueShards(files.flatMap((file) => shardsForFile(context.shards, file)));
    }
  }

  if (!selected.length) {
    return { text: `No occurrence of ${JSON.stringify(pattern)} was found in indexed UE 5.8 source files.`, isError: false };
  }
  const results = await mapConcurrent(selected, 2, (shard) => runWorker(context, shard, "find_all", {
    pattern,
    in: normalizeRepoPath(args.in),
    ignore_case: args.ignore_case === true,
    fixed: args.fixed === true,
    max_hits: Number.isFinite(args.max_hits) ? Math.max(1, Math.floor(args.max_hits)) : 300,
  }));
  const blocks = results.filter((result) => result && result.ok && result.totalHits > 0)
    .map((result) => `## [${result.shard}]\n${result.text}`);
  const totalHits = results.reduce((sum, result) => sum + (result && result.ok ? result.totalHits || 0 : 0), 0);
  blocks.push(`searched shards: ${selected.map((shard) => shard.name).join(", ")}\ntotal collected hits: ${totalHits}`);
  const errors = results.filter((result) => !result || !result.ok);
  if (errors.length) {
    blocks.push(`worker errors: ${errors.map((result) => `${result.shard}: ${result.error}`).join("; ")}`);
  }
  return { text: truncateCombined(blocks.join("\n\n")), isError: false };
}

async function handleRepoMap(context, args) {
  if (!args.shard) {
    return { text: shardSummary(context), isError: false };
  }
  const shard = resolveShardHint(context.shards, args.shard);
  if (!shard) {
    return { text: `Unknown or ambiguous shard "${args.shard}".`, isError: true };
  }
  const result = await runWorker(context, shard, "repo_map", {
    max_dirs: Number.isFinite(args.max_dirs) ? args.max_dirs : undefined,
  });
  return result.ok
    ? { text: `## [${shard.name}]\n${result.text}`, isError: false }
    : { text: result.error, isError: true };
}

async function handleFreshness(context, args) {
  if (!args.shard) {
    return {
      text: "graft_check_freshness requires shard because each UE 5.8 graph check scans the large engine tree. Use graft_repo_map to list shard names.",
      isError: true,
    };
  }
  const shard = resolveShardHint(context.shards, args.shard);
  if (!shard) {
    return { text: `Unknown or ambiguous shard "${args.shard}".`, isError: true };
  }
  const selected = [shard];
  const results = await mapConcurrent(selected, 2, (shard) => runWorker(context, shard, "check_freshness", {}));
  const blocks = results.map((result) => result && result.ok
    ? `## [${result.shard}]\n${result.text}`
    : `## [${result ? result.shard : "unknown"}]\nERROR: ${result ? result.error : "no result"}`);
  return { text: truncateCombined(blocks.join("\n\n")), isError: results.some((result) => !result || !result.ok) };
}

async function handleTool(context, name, args, modules) {
  switch (name) {
    case "graft_find_code":
      return handleFindCode(context, args, modules);
    case "graft_file_api":
      return handleFileApi(context, args);
    case "graft_trace_calls":
      return handleTraceCalls(context, args);
    case "graft_find_all":
      return handleFindAll(context, args);
    case "graft_repo_map":
      return handleRepoMap(context, args);
    case "graft_check_freshness":
      return handleFreshness(context, args);
    default:
      return { text: `Unknown tool: ${name}`, isError: true };
  }
}

function addShardSchema(tools) {
  return tools.map((tool) => {
    const copy = JSON.parse(JSON.stringify(tool));
    copy.inputSchema = copy.inputSchema || { type: "object", properties: {} };
    copy.inputSchema.properties = copy.inputSchema.properties || {};
    copy.inputSchema.properties.shard = {
      type: "string",
      description: copy.name === "graft_check_freshness"
        ? "required UE 5.8 shard name or unique name fragment; checked one shard at a time"
        : "optional UE 5.8 shard name or unique name fragment; omit for automatic routing",
    };
    if (copy.name === "graft_check_freshness") {
      copy.inputSchema.required = [
        ...new Set([...(copy.inputSchema.required || []), "shard"]),
      ];
    }
    if (["graft_find_code", "graft_trace_calls"].includes(copy.name)) {
      copy.inputSchema.properties.max_shards = {
        type: "number",
        description: `maximum automatically routed shards (default ${DEFAULT_MAX_SHARDS}, max 26)`,
      };
    }
    if (copy.name === "graft_find_all") {
      copy.inputSchema.properties.max_hits = {
        type: "number",
        description: "maximum collected hits per selected shard (default 300)",
      };
    }
    if (copy.name === "graft_repo_map") {
      copy.description = "List all UE 5.8 Graft shards. Pass shard to inspect that shard's detailed repo map.";
    } else {
      copy.description = `UE 5.8 federated wrapper. ${copy.description}`;
    }
    return copy;
  });
}

function mcpInstructions(modules, shardCount) {
  const upstreamInstructions = modules.mcpInstructions()
    .split(/\r?\n/)
    .filter((line) => !line.includes("refreshes before each query"))
    .join("\n")
    .replaceAll("mcp__graft__", "mcp__graft_ue58__")
    .trimEnd();
  return [
    `This server federates ${shardCount} read-only UE 5.8 Graft shards behind one tool set.`,
    "Use it only for Unreal Engine source; use the project Graft server for ccl8 code.",
    "Automatic routing uses literal symbols and file evidence before loading a shard. Include exact class, function, module, plugin, or file names when known. Pass shard when the target shard is known. Engine shards do not auto-rebuild; call graft_check_freshness explicitly and rebuild with the project Tools/Graft script when stale.",
    "",
    upstreamInstructions,
  ].join("\n");
}

async function runWorkerMode(encodedPayload) {
  const payload = JSON.parse(Buffer.from(encodedPayload, "base64url").toString("utf8"));
  const modules = await loadGraftModules();
  const args = payload.args || {};
  let result;

  switch (payload.tool) {
    case "find_code": {
      const engine = new modules.Graft({ contextDir: payload.indexDir });
      result = {
        ok: true,
        shard: payload.shard,
        result: engine.ask(payload.engineRoot, args.query, {
          limit: args.limit,
          source: true,
          full: args.full === true,
          in: args.in || undefined,
        }),
      };
      break;
    }
    case "file_api": {
      const value = modules.skeleton(payload.engineRoot, args.file, { contextDir: payload.indexDir });
      result = { ok: true, shard: payload.shard, text: modules.formatSkeleton(value) };
      break;
    }
    case "trace_calls": {
      const graph = modules.loadGraphCached(payload.indexDir);
      if (!graph) {
        result = { ok: false, shard: payload.shard, error: "graph not found" };
        break;
      }
      const inOption = args.in ? { in: args.in } : {};
      const matches = modules.resolveSymbol(graph, args.symbol, inOption);
      if (!matches.length) {
        result = { ok: true, shard: payload.shard, found: false, text: `no symbol "${args.symbol}" in shard` };
        break;
      }
      const direction = args.direction === "out" ? "out" : "in";
      const depth = args.depth === "all" || args.depth === "full"
        ? Number.POSITIVE_INFINITY
        : Number.isFinite(args.depth) && args.depth >= 1
          ? Math.floor(args.depth)
          : 1;
      const walked = matches.map((symbol) => ({
        symbol,
        hits: modules.edgeWalk(graph, symbol, direction, depth),
      }));
      const byId = new Map(walked.map((entry) => [entry.symbol.id, entry.hits]));
      const body = matches.map((symbol) => {
        const lines = [modules.headerOf(symbol)];
        const hits = byId.get(symbol.id) || [];
        if (!hits.length) {
          lines.push(modules.looseNoteFor(direction, symbol.name, matches.length));
        } else {
          for (const hit of hits) {
            lines.push(modules.hitLine(direction, hit, depth > 1));
          }
        }
        return lines.join("\n");
      }).join("\n\n");
      result = {
        ok: true,
        shard: payload.shard,
        found: true,
        text: modules.withSavings(body, modules.callersSavings(graph, walked)),
      };
      break;
    }
    case "find_all": {
      const graph = modules.loadGraphCached(payload.indexDir);
      if (!graph) {
        result = { ok: false, shard: payload.shard, error: "graph not found" };
        break;
      }
      const grepResult = modules.grepGraph(graph, payload.engineRoot, args.pattern, {
        ignoreCase: args.ignore_case,
        fixed: args.fixed,
        in: args.in || undefined,
        maxHits: args.max_hits,
      });
      result = {
        ok: true,
        shard: payload.shard,
        totalHits: grepResult.totalHits,
        text: grepResult.totalHits === 0
          ? modules.zeroHitNote(grepResult)
          : modules.formatGrepResult(grepResult),
      };
      break;
    }
    case "repo_map": {
      const graph = modules.loadGraphCached(payload.indexDir);
      if (!graph) {
        result = { ok: false, shard: payload.shard, error: "graph not found" };
        break;
      }
      result = {
        ok: true,
        shard: payload.shard,
        text: modules.formatRepoMap(modules.buildRepoMap(graph, { maxDirs: args.max_dirs })),
      };
      break;
    }
    case "check_freshness": {
      const engine = new modules.Graft({ contextDir: payload.indexDir });
      const graphCheck = await engine.checkGraph(payload.engineRoot);
      result = {
        ok: true,
        shard: payload.shard,
        text: modules.formatGraphCheckReport(graphCheck),
      };
      break;
    }
    default:
      result = { ok: false, shard: payload.shard, error: `unknown worker tool: ${payload.tool}` };
  }

  process.stdout.write(JSON.stringify(result));
}

async function createContext(options) {
  const indexRoot = normalizeAbsoluteDirectory(options.indexRoot, "UE58 Graft index root");
  const engineRoot = normalizeAbsoluteDirectory(options.engineRoot, "UE58 Engine root");
  const shards = discoverShards(indexRoot);
  const graftPackageRoot = findGraftPackageRoot();
  return { indexRoot, engineRoot, shards, graftPackageRoot };
}

function send(message) {
  process.stdout.write(`${JSON.stringify(message)}\n`);
}

function reply(id, result) {
  send({ jsonrpc: "2.0", id, result });
}

function replyError(id, code, message) {
  send({ jsonrpc: "2.0", id, error: { code, message } });
}

async function runMcpServer(context, modules) {
  const tools = addShardSchema(modules.TOOLS);
  const instructions = mcpInstructions(modules, context.shards.length);
  const rl = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
  let pending = 0;
  let inputEnded = false;

  const finishIfIdle = () => {
    if (inputEnded && pending === 0) {
      process.exit(0);
    }
  };

  rl.on("line", (line) => {
    const text = line.trim();
    if (!text) {
      return;
    }
    let message;
    try {
      message = JSON.parse(text);
    } catch {
      replyError(null, -32700, "parse error");
      return;
    }

    const { id, method, params } = message;
    const notification = id === undefined;
    switch (method) {
      case "initialize":
        if (!notification) {
          reply(id, {
            protocolVersion: params && params.protocolVersion ? params.protocolVersion : "2024-11-05",
            capabilities: { tools: {} },
            serverInfo: { name: "graft-ue58-federated", version: SERVER_VERSION },
            instructions,
          });
        }
        return;
      case "notifications/initialized":
      case "notifications/cancelled":
        return;
      case "ping":
        if (!notification) {
          reply(id, {});
        }
        return;
      case "tools/list":
        if (!notification) {
          reply(id, { tools });
        }
        return;
      case "tools/call":
        if (notification) {
          return;
        }
        pending += 1;
        handleTool(context, String(params && params.name || ""), params && params.arguments || {}, modules)
          .then((result) => reply(id, {
            content: [{ type: "text", text: result.text }],
            isError: result.isError === true,
          }))
          .catch((error) => replyError(id, -32603, error instanceof Error ? error.message : String(error)))
          .finally(() => {
            pending -= 1;
            finishIfIdle();
          });
        return;
      default:
        if (!notification) {
          replyError(id, -32601, `method not found: ${method}`);
        }
    }
  });

  process.stdin.on("end", () => {
    inputEnded = true;
    finishIfIdle();
  });
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (options.workerPayload) {
    await runWorkerMode(options.workerPayload);
    return;
  }

  const context = await createContext(options);
  const modules = await loadGraftModules();
  if (options.listShards) {
    process.stdout.write(`${shardSummary(context)}\n`);
    return;
  }
  if (options.probe) {
    const result = await handleFindCode(context, { query: options.probe, limit: DEFAULT_LIMIT }, modules);
    process.stdout.write(`${result.text}\n`);
    return;
  }
  await runMcpServer(context, modules);
}

main().catch((error) => {
  process.stderr.write(`${error instanceof Error ? error.stack || error.message : String(error)}\n`);
  process.exit(1);
});
