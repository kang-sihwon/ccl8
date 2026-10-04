// Plan and build the UE 5.8 engine Graft shards that graft_ue58_federated_mcp.cjs serves.
//
//   node ue58_shards.mjs plan  --engine-root <dir> --index-root <dir> [--budget-mb 170] [--extensions .h,.cpp,...]
//   node ue58_shards.mjs build --engine-root <dir> --index-root <dir> --shard <name> [--plan <file>]
//
// The builder calls the Graft engine API instead of the CLI. A Plugins shard scopes several
// hundred Source folders, and that many --only-dir flags exceed the Windows command-line
// limit. build_ue58_graft_indexes.ps1 drives both commands; see Docs/AgentContextTools.md.

import fs from "node:fs";
import path from "node:path";
import { execFileSync } from "node:child_process";
import { pathToFileURL } from "node:url";

// C/C++ plus the C# build rules (*.Build.cs, *.Target.cs). Applied through the
// GRAFT_ONLY_EXTENSIONS local patch; the CLI's -e flag never reaches the wiring build.
const DEFAULT_EXTENSIONS = ".h,.hpp,.hh,.c,.cc,.cxx,.cpp,.cs";
const DEFAULT_BUDGET_MB = 170;
const PLAN_FILE = "ue58_shard_plan.json";
// Written into each shard folder after a build so `plan` can tell when the shard on disk
// was cut from a different folder list than the current plan.
const SCOPE_FILE = ".ue58-shard-scope.json";
// Shard names the federated wrapper classifies by: Source_Runtime_Batched is matched
// exactly, Plugins_Batched_ and Source_ThirdParty_ by prefix.
const SINGLE_SHARDS = [
  ["Source_Runtime_Batched", "Runtime"],
  ["Source_Editor", "Editor"],
  ["Source_Developer", "Developer"],
  ["Source_Programs", "Programs"],
];
const PLUGIN_SKIP_DIRS = new Set(["Intermediate", "Binaries", "Saved", "Content", "Resources"]);
const PROGRESS_EVERY = 5000;
const MB = 1048576;

function fail(message, code = 1) {
  const error = new Error(message);
  error.exitCode = code;
  throw error;
}

function parseArguments(argv) {
  const options = {
    command: argv[0] || "",
    engineRoot: process.env.UE58_ENGINE_ROOT || "",
    indexRoot: process.env.UE58_GRAFT_INDEX_ROOT || "",
    budgetMb: DEFAULT_BUDGET_MB,
    extensions: DEFAULT_EXTENSIONS,
    shard: "",
    planFile: "",
    allowUnpatched: false,
  };
  for (let index = 1; index < argv.length; index += 1) {
    const arg = argv[index];
    const next = () => {
      index += 1;
      if (index >= argv.length) fail(`Missing value for ${arg}`, 3);
      return argv[index];
    };
    switch (arg) {
      case "--engine-root": options.engineRoot = next(); break;
      case "--index-root": options.indexRoot = next(); break;
      case "--budget-mb": options.budgetMb = Number(next()); break;
      case "--extensions": options.extensions = next(); break;
      case "--shard": options.shard = next(); break;
      case "--plan": options.planFile = next(); break;
      case "--allow-unpatched-graft": options.allowUnpatched = true; break;
      default: fail(`Unknown argument: ${arg}`, 3);
    }
  }
  if (!["plan", "build"].includes(options.command)) fail("Usage: ue58_shards.mjs plan|build ...", 3);
  if (!options.engineRoot) fail("--engine-root (or UE58_ENGINE_ROOT) is required.", 3);
  if (!options.indexRoot) fail("--index-root (or UE58_GRAFT_INDEX_ROOT) is required.", 3);
  if (!Number.isFinite(options.budgetMb) || options.budgetMb <= 0) fail("--budget-mb must be a positive number.", 3);
  options.engineRoot = path.resolve(options.engineRoot);
  options.indexRoot = path.resolve(options.indexRoot);
  if (!fs.existsSync(path.join(options.engineRoot, "Source"))) fail(`Engine Source folder was not found under ${options.engineRoot}`, 3);
  if (!options.planFile) options.planFile = path.join(options.indexRoot, ".build", PLAN_FILE);
  return options;
}

// Same lookup order as graft_ue58_federated_mcp.cjs.
function findGraftPackageRoot() {
  const candidates = [];
  if (process.env.GRAFT_PACKAGE_ROOT) candidates.push(process.env.GRAFT_PACKAGE_ROOT);
  if (process.env.APPDATA) candidates.push(path.join(process.env.APPDATA, "npm", "node_modules", "@nanonets", "graft"));
  try {
    const npmCommand = process.platform === "win32" ? "npm.cmd" : "npm";
    const npmRoot = execFileSync(npmCommand, ["root", "-g"], { encoding: "utf8", windowsHide: true, stdio: ["ignore", "pipe", "ignore"] }).trim();
    if (npmRoot) candidates.push(path.join(npmRoot, "@nanonets", "graft"));
  } catch {
    // APPDATA and GRAFT_PACKAGE_ROOT remain as candidates.
  }
  for (const candidate of candidates) {
    if (fs.existsSync(path.join(candidate, "dist", "engine.js"))) return path.resolve(candidate);
  }
  fail("The global @nanonets/graft package was not found. Run npm install -g @nanonets/graft@0.18.0.", 3);
}

// The three files the Orbis local patch touches. Without the patch a C/C++ build aborts
// after about 107 MB of parsed source and the extension filter is ignored.
function assertGraftPatched(packageRoot, allowUnpatched) {
  const files = ["ingest/fs.js", "graph/generic.js", "graph/source-files.js"];
  const missing = files.filter((file) => !fs.readFileSync(path.join(packageRoot, "dist", file), "utf8").includes("Orbis local patch"));
  if (missing.length === 0) return;
  const message = `Graft at ${packageRoot} lacks the Orbis local patch in: ${missing.join(", ")}. Run Tools/Graft/apply_graft_orbis_patch.cmd apply or pass --allow-unpatched-graft.`;
  if (!allowUnpatched) fail(message, 2);
  console.warn(`WARNING: ${message}`);
}

async function loadGraft(packageRoot) {
  const load = (relative) => import(pathToFileURL(path.join(packageRoot, "dist", relative)).href);
  const [engineModule, sourceFilesModule] = await Promise.all([load("engine.js"), load(path.join("graph", "source-files.js"))]);
  return { Graft: engineModule.Graft, listSourceFiles: sourceFilesModule.listSourceFiles };
}

const toPosix = (value) => value.split(path.sep).join("/");
const megabytes = (bytes) => Number((bytes / MB).toFixed(1));

// Every file Graft would parse under `root`, as engine-relative posix paths with sizes.
function listParsedFiles(listSourceFiles, engineRoot, root) {
  const files = [];
  for (const absolute of listSourceFiles(root, path.join(root, "__none__"))) {
    let size;
    try {
      size = fs.statSync(absolute).size;
    } catch {
      continue;
    }
    files.push({ rel: toPosix(path.relative(engineRoot, absolute)), size });
  }
  return files;
}

// Group files by the first matching root; roots must not nest.
function sumByRoot(files, roots) {
  const totals = new Map(roots.map((root) => [root, { files: 0, bytes: 0 }]));
  for (const file of files) {
    const root = roots.find((candidate) => file.rel === candidate || file.rel.startsWith(`${candidate}/`));
    if (!root) continue;
    const total = totals.get(root);
    total.files += 1;
    total.bytes += file.size;
  }
  return totals;
}

// Fill shards in ordinal path order; a new shard starts when the next unit would exceed
// the budget. Units above the budget get a shard of their own and a warning.
function packUnits(units, budgetBytes, prefix, warnings) {
  const shards = [];
  let current = null;
  for (const unit of units) {
    if (unit.files === 0) continue;
    if (unit.bytes > budgetBytes) warnings.push(`${unit.root} alone is ${megabytes(unit.bytes)} MB, above the ${megabytes(budgetBytes)} MB budget.`);
    if (!current || current.bytes + unit.bytes > budgetBytes) {
      current = { name: "", roots: [], files: 0, bytes: 0 };
      shards.push(current);
    }
    current.roots.push(unit.root);
    current.files += unit.files;
    current.bytes += unit.bytes;
  }
  shards.forEach((shard, index) => { shard.name = `${prefix}${String(index + 1).padStart(2, "0")}`; });
  return shards;
}

function planSingleShards(graft, engineRoot) {
  return SINGLE_SHARDS.map(([name, folder]) => {
    const files = listParsedFiles(graft.listSourceFiles, engineRoot, path.join(engineRoot, "Source", folder));
    return { name, roots: [`Source/${folder}`], files: files.length, bytes: files.reduce((sum, file) => sum + file.size, 0) };
  });
}

// The outermost Source folder of every plugin. Intermediate holds generated code and
// Content can hold headers, so both stay out; Source/ThirdParty inside a plugin stays in.
function findPluginSourceRoots(engineRoot) {
  const roots = [];
  const walk = (dir) => {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      if (!entry.isDirectory() || entry.name.startsWith(".") || PLUGIN_SKIP_DIRS.has(entry.name)) continue;
      const full = path.join(dir, entry.name);
      if (entry.name === "Source") roots.push(toPosix(path.relative(engineRoot, full)));
      else walk(full);
    }
  };
  walk(path.join(engineRoot, "Plugins"));
  return roots.sort();
}

function planPluginShards(graft, engineRoot, budgetBytes, warnings) {
  const roots = findPluginSourceRoots(engineRoot);
  const files = listParsedFiles(graft.listSourceFiles, engineRoot, path.join(engineRoot, "Plugins"));
  const totals = sumByRoot(files, roots);
  const units = roots.map((root) => ({ root, ...totals.get(root) }));
  return packUnits(units, budgetBytes, "Plugins_Batched_", warnings);
}

// One unit per child folder of Source/ThirdParty; a loose file directly under it is its own unit.
function planThirdPartyShards(graft, engineRoot, budgetBytes, warnings) {
  const files = listParsedFiles(graft.listSourceFiles, engineRoot, path.join(engineRoot, "Source", "ThirdParty"));
  const units = new Map();
  for (const file of files) {
    const parts = file.rel.split("/");
    const root = parts.length > 3 ? parts.slice(0, 3).join("/") : file.rel;
    const unit = units.get(root) ?? { root, files: 0, bytes: 0 };
    unit.files += 1;
    unit.bytes += file.size;
    units.set(root, unit);
  }
  return packUnits([...units.values()].sort((a, b) => (a.root < b.root ? -1 : a.root > b.root ? 1 : 0)), budgetBytes, "Source_ThirdParty_", warnings);
}

function runPlan(options, graft) {
  const budgetBytes = options.budgetMb * MB;
  const warnings = [];
  const shards = [
    ...planSingleShards(graft, options.engineRoot),
    ...planPluginShards(graft, options.engineRoot, budgetBytes, warnings),
    ...planThirdPartyShards(graft, options.engineRoot, budgetBytes, warnings),
  ].map((shard) => ({ name: shard.name, roots: shard.roots, files: shard.files, mb: megabytes(shard.bytes) }));

  const plan = {
    generatedAt: new Date().toISOString(),
    engineRoot: options.engineRoot,
    budgetMb: options.budgetMb,
    extensions: options.extensions,
    shards,
  };
  fs.mkdirSync(path.dirname(options.planFile), { recursive: true });
  fs.writeFileSync(options.planFile, `${JSON.stringify(plan, null, 1)}\n`);

  console.log(`plan: ${shards.length} shards, ${shards.reduce((sum, shard) => sum + shard.files, 0)} files, ${megabytes(shards.reduce((sum, shard) => sum + shard.mb * MB, 0))} MB -> ${options.planFile}`);
  for (const shard of shards) {
    const range = shard.roots.length === 1 ? shard.roots[0] : `${shard.roots.length} roots: ${shard.roots[0]} .. ${shard.roots.at(-1)}`;
    console.log(`  ${shard.name.padEnd(24)} ${String(shard.files).padStart(6)} files ${String(shard.mb).padStart(7)} MB  ${range}`);
  }
  for (const warning of warnings) console.log(`  WARNING: ${warning}`);

  // A shard built from a different folder list still answers queries; say so, because a
  // rebuild re-cuts it (Graft prunes the cards that leave the scope).
  for (const shard of shards) {
    const outDir = path.join(options.indexRoot, shard.name);
    if (!fs.existsSync(path.join(outDir, ".graph", "wiring.json"))) {
      console.log(`  ${shard.name}: not built yet`);
      continue;
    }
    const scopePath = path.join(outDir, SCOPE_FILE);
    if (!fs.existsSync(scopePath)) {
      console.log(`  ${shard.name}: built before scope tracking; its folder list is unknown until the next build`);
      continue;
    }
    const recorded = JSON.parse(fs.readFileSync(scopePath, "utf8"));
    const sameRoots = recorded.roots.length === shard.roots.length && recorded.roots.every((root, index) => root === shard.roots[index]);
    if (!sameRoots || recorded.extensions !== options.extensions) {
      console.log(`  ${shard.name}: scope on disk differs from this plan (${recorded.roots.length} -> ${shard.roots.length} roots); rebuild to re-cut it`);
    }
  }

  // Shard folders on disk that the plan no longer produces keep serving stale results.
  if (fs.existsSync(options.indexRoot)) {
    const planned = new Set(shards.map((shard) => shard.name));
    const orphans = fs.readdirSync(options.indexRoot, { withFileTypes: true })
      .filter((entry) => entry.isDirectory() && !entry.name.startsWith(".") && !planned.has(entry.name)
        && fs.existsSync(path.join(options.indexRoot, entry.name, ".graph", "wiring.json")))
      .map((entry) => entry.name);
    for (const orphan of orphans) console.log(`  WARNING: ${orphan} exists under the index root but is not in the plan; delete it or it stays in the federated shard list.`);
  }
}

async function runBuild(options, graft) {
  if (!options.shard) fail("--shard is required for build.", 3);
  if (!fs.existsSync(options.planFile)) fail(`Plan file not found: ${options.planFile}. Run the plan command first.`, 3);
  const plan = JSON.parse(fs.readFileSync(options.planFile, "utf8"));
  const shard = plan.shards.find((candidate) => candidate.name === options.shard);
  if (!shard) fail(`Shard ${options.shard} is not in ${options.planFile}.`, 3);

  const outDir = path.join(options.indexRoot, shard.name);
  const started = Date.now();
  const stamp = () => `[${((Date.now() - started) / 60000).toFixed(1)} min]`;
  console.log(`build ${shard.name}: ${shard.files} files, ${shard.mb} MB, ${shard.roots.length} root(s) -> ${outDir}`);

  let lastPhase = "";
  const result = await new graft.Graft({ contextDir: outDir }).graph(options.engineRoot, {
    onlyDirs: shard.roots,
    onProgress: ({ phase, index, total }) => {
      const count = index + 1;
      if (phase !== lastPhase || count % PROGRESS_EVERY === 0 || count === total) {
        lastPhase = phase;
        console.log(`  ${stamp()} ${phase} ${count}/${total}`);
      }
    },
  });

  const errors = result.errors ?? [];
  console.log(`  wiring: ${result.nodes} nodes, ${result.edges} edges, ${result.cards} cards [${result.languages.join(", ")}]`);
  console.log(`  parsed: ${result.parsed} of ${result.files} files (${result.reused} replayed from cache), ${errors.length} error(s)`);
  for (const error of errors.slice(0, 20)) console.log(`  error: ${error}`);
  if (errors.length > 20) console.log(`  ... ${errors.length - 20} more error(s)`);

  const wiring = path.join(outDir, ".graph", "wiring.json");
  const wiringMb = fs.existsSync(wiring) ? megabytes(fs.statSync(wiring).size) : null;
  const extractCache = fs.existsSync(path.join(outDir, ".cache")) && fs.readdirSync(path.join(outDir, ".cache")).some((name) => name.startsWith("extract"));
  console.log(`  ${stamp()} wiring.json ${wiringMb === null ? "missing" : `${wiringMb} MB`}, extract cache ${extractCache ? "present" : "absent (too large for one JSON string; the next build re-parses everything)"}`);

  // A WASM abort cascade means the parser-lifetime patch is missing or Graft was reinstalled.
  if (errors.some((error) => error.includes("Aborted()"))) fail(`${shard.name}: grammar aborts detected; re-apply the Graft local patch and rebuild with a fresh output folder.`, 2);
  if (wiringMb === null) fail(`${shard.name}: build finished without wiring.json.`, 1);
  fs.writeFileSync(path.join(outDir, SCOPE_FILE), `${JSON.stringify({ builtAt: new Date().toISOString(), extensions: options.extensions, roots: shard.roots }, null, 1)}\n`);
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  process.env.GRAFT_ONLY_EXTENSIONS = options.extensions;
  process.env.GRAFT_NO_GITIGNORE = "1"; // never write .gitignore or .ignore into the engine tree
  process.env.GRAFT_NO_IGNORE = "1";
  const packageRoot = findGraftPackageRoot();
  assertGraftPatched(packageRoot, options.allowUnpatched);
  const graft = await loadGraft(packageRoot);
  if (options.command === "plan") runPlan(options, graft);
  else await runBuild(options, graft);
}

main().catch((error) => {
  process.stderr.write(`ERROR: ${error instanceof Error ? error.message : String(error)}\n`);
  process.exit(error?.exitCode ?? 1);
});
