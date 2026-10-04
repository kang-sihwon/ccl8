[CmdletBinding()]
param(
    # plan: write the shard plan only. Other values build the matching shards after planning.
    [Parameter(Position = 0)]
    [ValidateSet("plan", "runtime", "editor", "developer", "programs", "plugins", "thirdparty", "all")]
    [string]$Target = "plan",

    # Engine folder that holds Source\ and Plugins\. Defaults to UE58_ENGINE_ROOT, then to
    # the Engine folder beside this repository (the source-tree layout used on the home PC).
    [string]$EngineRoot = $env:UE58_ENGINE_ROOT,

    # Folder that receives one sub-folder per shard. Defaults to UE58_GRAFT_INDEX_ROOT.
    [string]$IndexRoot = $env:UE58_GRAFT_INDEX_ROOT,

    # Build only the shards whose name starts with this text, e.g. Source_ThirdParty_03.
    [string]$Shard = "",

    # Parsed source per Plugins or ThirdParty shard. 170 MB keeps wiring.json under the
    # 512 MB JSON string limit at the ratios measured on 2026-09-16 (up to 2.42x).
    [ValidateRange(50, 400)]
    [int]$BudgetMB = 170,

    # Node heap cap for one shard build; a 210 MB shard peaked near 3 GB.
    [ValidateRange(2048, 262144)]
    [int]$NodeHeapMB = 16384,

    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Read machine paths from ccl8's ignored local configuration when no override is supplied.
$localConfigPath = Join-Path $PSScriptRoot '..\..\.local\agent-paths.json'
if (Test-Path -LiteralPath $localConfigPath) {
    $localConfig = Get-Content -LiteralPath $localConfigPath -Raw | ConvertFrom-Json
    if (-not $EngineRoot) { $EngineRoot = $localConfig.engineRoot }
    if (-not $IndexRoot) { $IndexRoot = $localConfig.indexRoot }
    if (-not $env:GRAFT_PACKAGE_ROOT) {
        $repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
        $runtimePath = $localConfig.graftPackageRoot
        if (-not [IO.Path]::IsPathRooted($runtimePath)) { $runtimePath = Join-Path $repositoryRoot $runtimePath }
        $env:GRAFT_PACKAGE_ROOT = [IO.Path]::GetFullPath($runtimePath)
    }
}

$script:ShardsByTarget = @{
    runtime    = @("Source_Runtime_Batched")
    editor     = @("Source_Editor")
    developer  = @("Source_Developer")
    programs   = @("Source_Programs")
    plugins    = @("Plugins_Batched_")
    thirdparty = @("Source_ThirdParty_")
}

function Resolve-EngineRoot {
    param([string]$Requested)

    if (-not [string]::IsNullOrWhiteSpace($Requested)) {
        return (Resolve-Path -LiteralPath $Requested).Path.TrimEnd("\")
    }

    # Tools\Graft -> repository root -> its parent holds Engine\ in the source-tree layout.
    $sibling = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "..\Engine"
    if (Test-Path -LiteralPath (Join-Path $sibling "Source") -PathType Container) {
        return (Resolve-Path -LiteralPath $sibling).Path.TrimEnd("\")
    }

    throw "Pass -EngineRoot or set UE58_ENGINE_ROOT. No Engine folder was found beside the repository."
}

function Get-NodePath {
    $node = Get-Command node.exe -ErrorAction SilentlyContinue
    if ($null -eq $node) {
        $node = Get-Command node -ErrorAction SilentlyContinue
    }
    if ($null -eq $node) {
        throw "node.exe was not found in PATH."
    }
    return $node.Source
}

function Invoke-ShardScript {
    param(
        [string]$Node,
        [string[]]$NodeFlags,
        [string[]]$Arguments
    )

    $script = Join-Path $PSScriptRoot "ue58_shards.mjs"
    if (-not (Test-Path -LiteralPath $script -PathType Leaf)) {
        throw "Shard script was not found: $script"
    }

    # The script prints its own progress on stdout, so no stderr redirection is needed
    # (Windows PowerShell 5.1 wraps redirected native stderr as error records). Stdout
    # goes to the host so this function returns only the exit code.
    & $Node @NodeFlags $script @Arguments | ForEach-Object { Write-Host $_ }
    return $LASTEXITCODE
}

function Select-Shards {
    param(
        [object[]]$Planned,
        [string]$TargetName,
        [string]$Prefix
    )

    $selected = @($Planned)
    if ($TargetName -ne "all") {
        $patterns = $script:ShardsByTarget[$TargetName]
        $selected = @($selected | Where-Object {
            $name = $_.name
            $null -ne ($patterns | Where-Object { $name.StartsWith($_, [System.StringComparison]::Ordinal) })
        })
    }
    if (-not [string]::IsNullOrWhiteSpace($Prefix)) {
        $selected = @($selected | Where-Object { $_.name.StartsWith($Prefix, [System.StringComparison]::Ordinal) })
    }
    return $selected
}

try {
    if ([string]::IsNullOrWhiteSpace($IndexRoot)) {
        throw "Pass -IndexRoot or set UE58_GRAFT_INDEX_ROOT."
    }

    $resolvedEngineRoot = Resolve-EngineRoot $EngineRoot
    $resolvedIndexRoot = [System.IO.Path]::GetFullPath($IndexRoot).TrimEnd("\")
    New-Item -ItemType Directory -Path $resolvedIndexRoot -Force | Out-Null
    $node = Get-NodePath
    $planFile = Join-Path $resolvedIndexRoot ".build\ue58_shard_plan.json"
    $common = @("--engine-root", $resolvedEngineRoot, "--index-root", $resolvedIndexRoot, "--plan", $planFile)

    Write-Host "Engine: $resolvedEngineRoot"
    Write-Host "Index:  $resolvedIndexRoot"
    Write-Host "Budget: $BudgetMB MB per Plugins/ThirdParty shard"
    Write-Host ""

    $planExit = Invoke-ShardScript -Node $node -NodeFlags @() -Arguments (@("plan") + $common + @("--budget-mb", $BudgetMB))
    if ($planExit -ne 0) {
        throw "Planning failed with exit code $planExit."
    }

    if ($Target -eq "plan" -or $DryRun) {
        Write-Host ""
        Write-Host "Plan written; nothing built."
        exit 0
    }

    $plan = Get-Content -LiteralPath $planFile -Raw | ConvertFrom-Json
    # @() keeps a single match an array; PowerShell unrolls one-element arrays on return.
    $shards = @(Select-Shards -Planned $plan.shards -TargetName $Target -Prefix $Shard)
    if ($shards.Count -eq 0) {
        throw "No planned shard matches target '$Target' and prefix '$Shard'."
    }

    Write-Host ""
    Write-Host ("Building {0} shard(s): {1}" -f $shards.Count, (($shards | ForEach-Object { $_.name }) -join ", "))

    # Not $shard: variable names are case-insensitive, so that would reuse the [string]$Shard
    # parameter and stringify each planned shard object.
    $results = @()
    foreach ($planned in $shards) {
        Write-Host ""
        Write-Host "===== $($planned.name) ====="
        $watch = [System.Diagnostics.Stopwatch]::StartNew()
        $exit = Invoke-ShardScript -Node $node -NodeFlags @("--max-old-space-size=$NodeHeapMB") -Arguments (@("build") + $common + @("--shard", $planned.name))
        $watch.Stop()
        $results += [pscustomobject]@{
            Shard   = $planned.name
            Minutes = [math]::Round($watch.Elapsed.TotalMinutes, 1)
            Exit    = $exit
        }
        if ($exit -ne 0) {
            throw "$($planned.name) failed with exit code $exit. Shards built before it are complete."
        }
    }

    Write-Host ""
    Write-Host "Completed successfully."
    $results | Format-Table -AutoSize | Out-String | Write-Host
    exit 0
}
catch {
    [Console]::Error.WriteLine("ERROR: " + $_.Exception.Message)
    exit 1
}
