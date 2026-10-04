[CmdletBinding()]
param(
    [string]$EngineRoot,
    [string]$IndexRoot,
    [string]$GraftPackageRoot,
    [string]$VaultRoot,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$localRoot = Join-Path $projectRoot '.local'
$settingsPath = Join-Path $localRoot 'agent-paths.json'
$codexPath = Join-Path $projectRoot '.codex\config.toml'
$claudePath = Join-Path $projectRoot '.claude\settings.local.json'

foreach ($target in @($settingsPath, $codexPath, $claudePath)) {
    if ((Test-Path -LiteralPath $target) -and -not $Force) {
        throw "Local configuration already exists: $target. Review it before using -Force."
    }
}
if (-not $EngineRoot) { $EngineRoot = Join-Path $projectRoot '..\Engine' }
$engine = (Resolve-Path -LiteralPath $EngineRoot).Path
if (-not (Test-Path -LiteralPath (Join-Path $engine 'Build\Build.version'))) {
    throw 'EngineRoot must name the Engine folder, not its parent.'
}
if (-not $IndexRoot) { throw 'Pass -IndexRoot with the existing UE 5.8 Graft index directory.' }
$indexes = (Resolve-Path -LiteralPath $IndexRoot).Path
$runtime = if ($GraftPackageRoot) { (Resolve-Path -LiteralPath $GraftPackageRoot).Path } else { '.local/graft-runtime/node_modules/@nanonets/graft' }
$vault = if ($VaultRoot) { (Resolve-Path -LiteralPath $VaultRoot).Path } else { '' }
$settings = [ordered]@{ engineRoot=$engine; indexRoot=$indexes; graftPackageRoot=$runtime; vaultRoot=$vault }
$slashRoot = $projectRoot.Replace('\','/')
$launcher = "$slashRoot/Tools/Agent/graft.cjs"
$codex = @"
# Generated for this machine; ignored by Git.
[mcp_servers.graft]
command = "node"
args = ["$launcher", "project"]
cwd = "$slashRoot"

[mcp_servers.graft_ue58]
command = "node"
args = ["$launcher", "engine"]
cwd = "$slashRoot"
"@
$enginePattern = '//' + $engine.Replace('\','/').Replace(':','') + '/**'
$directories = @($engine)
if ($vault) { $directories += $vault }
$claude = @{ permissions = @{
    additionalDirectories = $directories
    allow = @("Read($enginePattern)")
    deny = @("Edit($enginePattern)", "Write($enginePattern)")
} }
$utf8 = [Text.UTF8Encoding]::new($false)
New-Item -ItemType Directory -Path $localRoot,(Split-Path $codexPath),(Split-Path $claudePath) -Force | Out-Null
[IO.File]::WriteAllText($settingsPath, ($settings | ConvertTo-Json), $utf8)
[IO.File]::WriteAllText($codexPath, $codex + [Environment]::NewLine, $utf8)
[IO.File]::WriteAllText($claudePath, ($claude | ConvertTo-Json -Depth 6), $utf8)
Write-Host 'Local paths and project MCP settings prepared. Restart the agents to load them.'
