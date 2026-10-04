[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("setup", "status", "build", "query")]
    [string]$Command = "status",

    [Parameter(Position = 1)]
    [string]$Target,

    [Parameter(Position = 2, ValueFromRemainingArguments = $true)]
    [string[]]$Remaining
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$DocsRoot = Join-Path $ProjectRoot "Docs"
$LocalRoot = Join-Path $ProjectRoot ".graphify-local"

function Get-ToolPath {
    param([Parameter(Mandatory)][string]$Name)

    $commandInfo = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $commandInfo) {
        return $null
    }

    return $commandInfo.Source
}

function Update-ProcessPath {
    $candidateDirectories = @(
        (Join-Path $env:USERPROFILE ".local\bin"),
        (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Links")
    )

    foreach ($directory in $candidateDirectories) {
        if ((Test-Path -LiteralPath $directory) -and
            -not (($env:PATH -split ";") -contains $directory)) {
            $env:PATH = "$directory;$env:PATH"
        }
    }
}

function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Content
    )

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($Path, $Content, $encoding)
}

function Remove-GeneratedGraphifySection {
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    $content = [IO.File]::ReadAllText($Path)
    $pattern = '(?ms)(?:\r?\n)*## graphify\r?\n\r?\nThis project has a knowledge graph at graphify-out/.*\z'
    $updated = [Text.RegularExpressions.Regex]::Replace($content, $pattern, "`r`n")
    if ($updated -ne $content) {
        Write-Utf8NoBom -Path $Path -Content $updated
        Write-Host "Removed Graphify's generated always-on rules from $Path"
    }
}

function Set-GraphifySkillScope {
    $scopedDescription = 'description: "Use when the user explicitly invokes /graphify, or when ccl8 work requires relationships, omissions, or impact analysis across multiple project documents. Do not use for code lookup, symbols, call flow, or build errors; use Graft for those."'
    $skillPaths = @(
        (Join-Path $ProjectRoot ".claude\skills\graphify\SKILL.md"),
        (Join-Path $ProjectRoot ".codex\skills\graphify\SKILL.md")
    )

    foreach ($skillPath in $skillPaths) {
        if (-not (Test-Path -LiteralPath $skillPath)) {
            continue
        }

        $content = [IO.File]::ReadAllText($skillPath)
        if ($content -notmatch '(?m)^name:\s*graphify\s*$') {
            throw "Unexpected Graphify skill format: $skillPath"
        }

        $updated = [Text.RegularExpressions.Regex]::Replace(
            $content,
            '(?m)^description:.*$',
            $scopedDescription,
            1
        )
        if ($updated -ne $content) {
            Write-Utf8NoBom -Path $skillPath -Content $updated
            Write-Host "Scoped Graphify skill routing for ccl8: $skillPath"
        }
    }
}

function Remove-GraphifyHooksFromJson {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string[]]$CommandPrefixes,
        [switch]$RemoveWhenEmpty
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    $document = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    $hooksProperty = $document.PSObject.Properties["hooks"]
    if ($null -eq $hooksProperty) {
        return
    }

    $preToolUseProperty = $document.hooks.PSObject.Properties["PreToolUse"]
    if ($null -eq $preToolUseProperty) {
        return
    }

    $changed = $false
    $remainingEntries = @()
    foreach ($entry in @($document.hooks.PreToolUse)) {
        $remainingHooks = @()
        foreach ($hook in @($entry.hooks)) {
            $command = [string]$hook.command
            $isGraphifyHook = $false
            foreach ($prefix in $CommandPrefixes) {
                if ($command.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
                    $isGraphifyHook = $true
                    break
                }
            }

            if ($isGraphifyHook) {
                $changed = $true
            }
            else {
                $remainingHooks += $hook
            }
        }

        if ($remainingHooks.Count -gt 0) {
            $entry.hooks = $remainingHooks
            $remainingEntries += $entry
        }
    }

    if (-not $changed) {
        return
    }

    if ($remainingEntries.Count -gt 0) {
        $document.hooks.PreToolUse = $remainingEntries
    }
    else {
        $document.hooks.PSObject.Properties.Remove("PreToolUse")
    }

    $hookPropertyCount = @($document.hooks.PSObject.Properties).Count
    $documentPropertyCount = @($document.PSObject.Properties).Count
    if ($RemoveWhenEmpty -and $hookPropertyCount -eq 0 -and
        $documentPropertyCount -eq 1) {
        Remove-Item -LiteralPath $Path -Force
        Write-Host "Removed Graphify's generated hook file: $Path"
        return
    }

    $json = $document | ConvertTo-Json -Depth 100 -Compress
    Write-Utf8NoBom -Path $Path -Content ($json + "`r`n")

    $nodePath = Get-ToolPath -Name "node"
    if ($nodePath) {
        $nodeScript = 'const fs=require(String.fromCharCode(102,115));const p=process.argv[1];const data=JSON.parse(fs.readFileSync(p,String.fromCharCode(117,116,102,56)));fs.writeFileSync(p,JSON.stringify(data,null,2)+String.fromCharCode(10));'
        $global:LASTEXITCODE = 0
        & $nodePath -e $nodeScript $Path
        if ($LASTEXITCODE -ne 0) {
            throw "node failed to format JSON after removing Graphify hooks (exit code $LASTEXITCODE)."
        }
    }
    Write-Host "Removed Graphify's generated PreToolUse hooks from $Path"
}

function Remove-GraphifyConflictingIntegration {
    Remove-GeneratedGraphifySection -Path (Join-Path $ProjectRoot "AGENTS.md")
    Remove-GeneratedGraphifySection -Path (Join-Path $ProjectRoot "CLAUDE.md")

    $registrationPath = Join-Path $ProjectRoot ".claude\CLAUDE.md"
    if (Test-Path -LiteralPath $registrationPath) {
        $registration = ([IO.File]::ReadAllText($registrationPath) -replace "`r`n", "`n").Trim()
        $expectedRegistration = (@'
# graphify
- **graphify** (`.claude/skills/graphify/SKILL.md`) - any input to knowledge graph. Trigger: `/graphify`
When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.
'@ -replace "`r`n", "`n").Trim()
        if ($registration -eq $expectedRegistration) {
            Remove-Item -LiteralPath $registrationPath -Force
            Write-Host "Removed Graphify's generated Claude registration file."
        }
    }

    Remove-GraphifyHooksFromJson `
        -Path (Join-Path $ProjectRoot ".claude\settings.json") `
        -CommandPrefixes @("graphify hook-guard")
    Remove-GraphifyHooksFromJson `
        -Path (Join-Path $ProjectRoot ".codex\hooks.json") `
        -CommandPrefixes @("graphify hook-check") `
        -RemoveWhenEmpty

    $settingsBackup = Join-Path $ProjectRoot ".claude\settings.json.graphify-bak"
    if (Test-Path -LiteralPath $settingsBackup) {
        Remove-Item -LiteralPath $settingsBackup -Force
        Write-Host "Removed Graphify's generated Claude settings backup."
    }

    $conflictChecks = @(
        [PSCustomObject]@{ Path = (Join-Path $ProjectRoot "AGENTS.md"); Pattern = '(?m)^## graphify\s*$' },
        [PSCustomObject]@{ Path = (Join-Path $ProjectRoot "CLAUDE.md"); Pattern = '(?m)^## graphify\s*$' },
        [PSCustomObject]@{ Path = (Join-Path $ProjectRoot ".claude\CLAUDE.md"); Pattern = 'When the user types `/graphify`' },
        [PSCustomObject]@{ Path = (Join-Path $ProjectRoot ".claude\settings.json"); Pattern = 'graphify hook-guard' },
        [PSCustomObject]@{ Path = (Join-Path $ProjectRoot ".codex\hooks.json"); Pattern = 'graphify hook-check' }
    )
    foreach ($check in $conflictChecks) {
        if ((Test-Path -LiteralPath $check.Path) -and
            ([IO.File]::ReadAllText($check.Path) -match $check.Pattern)) {
            throw "Graphify's always-on integration remains after cleanup: $($check.Path)"
        }
    }
}

function Install-UvFromReleaseArchive {
    $architecture = if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
        "aarch64"
    }
    else {
        "x86_64"
    }
    $assetName = "uv-$architecture-pc-windows-msvc.zip"
    $downloadUri = "https://github.com/astral-sh/uv/releases/latest/download/$assetName"
    $temporaryBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd("\", "/")
    $temporaryRoot = Join-Path $temporaryBase ("ccl8-uv-install-" + [Guid]::NewGuid().ToString("N"))
    $resolvedTemporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
    if (-not $resolvedTemporaryRoot.StartsWith("$temporaryBase$([IO.Path]::DirectorySeparatorChar)", [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to use an unexpected temporary path: $resolvedTemporaryRoot"
    }

    try {
        New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null
        $archivePath = Join-Path $temporaryRoot $assetName
        Write-Host "Downloading $downloadUri ..."
        Invoke-WebRequest -Uri $downloadUri -OutFile $archivePath
        Expand-Archive -LiteralPath $archivePath -DestinationPath $temporaryRoot -Force

        $installDirectory = Join-Path $env:USERPROFILE ".local\bin"
        New-Item -ItemType Directory -Path $installDirectory -Force | Out-Null
        foreach ($binaryName in @("uv.exe", "uvx.exe", "uvw.exe")) {
            $binary = Get-ChildItem -LiteralPath $temporaryRoot -Recurse -File -Filter $binaryName |
                Select-Object -First 1
            if ($null -ne $binary) {
                Copy-Item -LiteralPath $binary.FullName -Destination (Join-Path $installDirectory $binaryName) -Force
            }
        }

        if (-not (Test-Path -LiteralPath (Join-Path $installDirectory "uv.exe"))) {
            throw "The uv release archive did not contain uv.exe."
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporaryRoot) {
            Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
        }
    }
}

function Assert-SafeLocalPath {
    param([Parameter(Mandatory)][string]$Path)

    $resolvedProject = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd("\", "/")
    $resolvedPath = [IO.Path]::GetFullPath($Path)
    $prefix = "$resolvedProject$([IO.Path]::DirectorySeparatorChar)"

    if (-not $resolvedPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify a path outside the project: $resolvedPath"
    }
}

function Get-ProjectRelativePath {
    param([Parameter(Mandatory)][string]$Path)

    $resolvedProject = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd("\", "/")
    $resolvedPath = [IO.Path]::GetFullPath($Path)
    $prefix = "$resolvedProject$([IO.Path]::DirectorySeparatorChar)"
    if (-not $resolvedPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the project: $resolvedPath"
    }

    return $resolvedPath.Substring($prefix.Length)
}

function Get-CorpusFiles {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("canonical", "research")]
        [string]$Kind
    )

    $allMarkdown = Get-ChildItem -LiteralPath $DocsRoot -Recurse -File -Filter "*.md"
    foreach ($file in $allMarkdown) {
        $relativePath = Get-ProjectRelativePath -Path $file.FullName
        $normalized = $relativePath.Replace("/", "\")
        $isImages = $normalized -match "^Docs\\Images(\\|$)"
        $isResearch = $normalized -match "^Docs\\Narrative\\(Research|Drafts|Reviews)(\\|$)"

        if ($Kind -eq "canonical" -and -not $isImages -and -not $isResearch) {
            $file
        }
        elseif ($Kind -eq "research" -and $isResearch) {
            $file
        }
    }
}

function Get-GraphPath {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("canonical", "research")]
        [string]$Kind
    )

    return Join-Path $LocalRoot "$Kind\graphify-out\graph.json"
}

function Get-GraphState {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("canonical", "research")]
        [string]$Kind
    )

    $files = @(Get-CorpusFiles -Kind $Kind)
    $graphPath = Get-GraphPath -Kind $Kind
    $newestSource = $files |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1

    $exists = Test-Path -LiteralPath $graphPath
    $stale = $false
    if ($exists -and $null -ne $newestSource) {
        $stale = $newestSource.LastWriteTimeUtc -gt (Get-Item -LiteralPath $graphPath).LastWriteTimeUtc
    }

    [PSCustomObject]@{
        Kind = $Kind
        FileCount = $files.Count
        GraphPath = $graphPath
        Exists = $exists
        Stale = $stale
        NewestSource = if ($null -eq $newestSource) { $null } else { $newestSource.FullName }
    }
}

function Show-Status {
    Update-ProcessPath
    $uvPath = Get-ToolPath -Name "uv"
    $graphifyPath = Get-ToolPath -Name "graphify"
    $claudePath = Get-ToolPath -Name "claude"

    Write-Host "Project: $ProjectRoot"
    Write-Host ("uv: " + $(if ($uvPath) { $uvPath } else { "MISSING" }))
    Write-Host ("graphify: " + $(if ($graphifyPath) { $graphifyPath } else { "MISSING" }))
    Write-Host ("claude: " + $(if ($claudePath) { $claudePath } else { "MISSING (required only for build)" }))
    Write-Host ("Claude skill: " + (Test-Path -LiteralPath (Join-Path $ProjectRoot ".claude\skills\graphify\SKILL.md")))
    Write-Host ("Codex skill: " + (Test-Path -LiteralPath (Join-Path $ProjectRoot ".codex\skills\graphify\SKILL.md")))

    foreach ($kind in @("canonical", "research")) {
        $state = Get-GraphState -Kind $kind
        $stateText = if (-not $state.Exists) {
            "missing"
        }
        elseif ($state.Stale) {
            "stale"
        }
        else {
            "ready"
        }

        Write-Host ("{0}: {1} Markdown files, {2}" -f $kind, $state.FileCount, $stateText)
        Write-Host ("  graph: {0}" -f $state.GraphPath)
    }
}

function Install-Graphify {
    Update-ProcessPath
    Remove-GraphifyConflictingIntegration
    $uvPath = Get-ToolPath -Name "uv"
    if (-not $uvPath) {
        $wingetPath = Get-ToolPath -Name "winget"
        if ($wingetPath) {
            Write-Host "Installing uv with winget..."
            & $wingetPath install -e --id astral-sh.uv --accept-package-agreements --accept-source-agreements
            if ($LASTEXITCODE -ne 0) {
                throw "winget failed to install uv (exit code $LASTEXITCODE)."
            }
        }
        else {
            Write-Host "winget was not found. Installing uv with Astral's official Windows installer..."
            $windowsPowerShell = Join-Path $PSHOME "powershell.exe"
            & $windowsPowerShell -NoLogo -NoProfile -ExecutionPolicy Bypass -Command `
                "Invoke-RestMethod https://astral.sh/uv/install.ps1 | Invoke-Expression"
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Astral's installer script could not run in this PowerShell environment. Falling back to the official GitHub release archive."
                Install-UvFromReleaseArchive
            }
        }

        Update-ProcessPath
        $uvPath = Get-ToolPath -Name "uv"
        if (-not $uvPath) {
            throw "uv was installed but is not on PATH. Run 'uv tool update-shell' or open a new terminal, then rerun setup."
        }
    }

    $previousErrorPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $installedTools = (& $uvPath tool list 2>&1 | Out-String)
    $toolListExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorPreference
    if ($toolListExitCode -ne 0 -and $installedTools -notmatch "No tools installed") {
        throw "uv tool list failed: $installedTools"
    }

    if ($installedTools -match "(?m)^graphifyy\s") {
        Write-Host "Upgrading graphifyy..."
        & $uvPath tool upgrade graphifyy
    }
    else {
        Write-Host "Installing graphifyy..."
        & $uvPath tool install graphifyy
    }
    if ($LASTEXITCODE -ne 0) {
        throw "uv failed to install or upgrade graphifyy (exit code $LASTEXITCODE)."
    }

    Update-ProcessPath
    $graphifyPath = Get-ToolPath -Name "graphify"
    if (-not $graphifyPath) {
        $toolBin = (& $uvPath tool dir --bin 2>$null | Select-Object -First 1)
        if ($toolBin -and (Test-Path -LiteralPath $toolBin)) {
            $env:PATH = "$toolBin;$env:PATH"
            $graphifyPath = Get-ToolPath -Name "graphify"
        }
    }
    if (-not $graphifyPath) {
        throw "graphifyy is installed but graphify is not on PATH. Run 'uv tool update-shell', open a new terminal, and rerun setup."
    }

    Push-Location $ProjectRoot
    try {
        Write-Host "Installing the Claude Code project skill..."
        $global:LASTEXITCODE = 0
        & $graphifyPath install --project --platform windows
        if ($LASTEXITCODE -ne 0) {
            throw "Claude Code project skill installation failed (exit code $LASTEXITCODE)."
        }

        Write-Host "Installing the Codex project skill..."
        $global:LASTEXITCODE = 0
        & $graphifyPath install --project --platform codex
        if ($LASTEXITCODE -ne 0) {
            throw "Codex project skill installation failed (exit code $LASTEXITCODE)."
        }
    }
    finally {
        Pop-Location
        Set-GraphifySkillScope
        Remove-GraphifyConflictingIntegration
    }

    & $graphifyPath --version
    Show-Status
}

function Copy-CorpusToStage {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("canonical", "research")]
        [string]$Kind
    )

    $stageRoot = Join-Path $LocalRoot "staging\$Kind"
    Assert-SafeLocalPath -Path $stageRoot

    if (Test-Path -LiteralPath $stageRoot) {
        Remove-Item -LiteralPath $stageRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null

    $files = @(Get-CorpusFiles -Kind $Kind)
    if ($files.Count -eq 0) {
        throw "No Markdown files matched the '$Kind' corpus."
    }

    foreach ($file in $files) {
        $relativePath = Get-ProjectRelativePath -Path $file.FullName
        $destination = Join-Path $stageRoot $relativePath
        $destinationDirectory = Split-Path -Parent $destination
        New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
        # Perforce keeps unopened files read-only and Copy-Item preserves that attribute.
        Set-ItemProperty -LiteralPath $destination -Name IsReadOnly -Value $false
        [IO.File]::SetLastWriteTimeUtc($destination, $file.LastWriteTimeUtc)
    }

    Write-Host ("Staged {0} Markdown files for {1}." -f $files.Count, $Kind)
    return $stageRoot
}

function Build-Graph {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("canonical", "research")]
        [string]$Kind
    )

    Update-ProcessPath
    $graphifyPath = Get-ToolPath -Name "graphify"
    if (-not $graphifyPath) {
        throw "graphify was not found. Run '.\Tools\Graphify\graphify_ccl8.cmd setup' first."
    }
    if (-not (Get-ToolPath -Name "claude")) {
        throw "claude was not found on PATH. Build from a terminal where 'claude --version' succeeds."
    }

    $stageRoot = Copy-CorpusToStage -Kind $Kind
    $outputRoot = Join-Path $LocalRoot $Kind
    Assert-SafeLocalPath -Path $outputRoot
    New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

    Write-Host "Building $Kind graph..."
    & $graphifyPath extract $stageRoot --backend claude-cli --mode deep --out $outputRoot --timing
    if ($LASTEXITCODE -ne 0) {
        throw "Graphify failed to build the '$Kind' graph (exit code $LASTEXITCODE)."
    }

    $graphPath = Get-GraphPath -Kind $Kind
    if (-not (Test-Path -LiteralPath $graphPath)) {
        throw "Graphify completed without creating the expected graph: $graphPath"
    }

    Write-Host "Graph ready: $graphPath"
}

function Query-Graph {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("canonical", "research", "both")]
        [string]$Kind,

        [Parameter(Mandatory)]
        [string]$Question
    )

    Update-ProcessPath
    $graphifyPath = Get-ToolPath -Name "graphify"
    if (-not $graphifyPath) {
        throw "graphify was not found. Run '.\Tools\Graphify\graphify_ccl8.cmd setup' first."
    }

    $kinds = if ($Kind -eq "both") { @("canonical", "research") } else { @($Kind) }
    $queried = 0
    foreach ($graphKind in $kinds) {
        $state = Get-GraphState -Kind $graphKind
        if (-not $state.Exists) {
            Write-Warning "$graphKind graph is missing. Continue with source documents or run 'graphify_ccl8.cmd build $graphKind'."
            continue
        }
        if ($state.Stale) {
            Write-Warning "$graphKind graph is stale. Results are leads only; verify the source documents before answering."
        }

        Write-Host ""
        Write-Host "===== $graphKind ====="
        & $graphifyPath query $Question --graph $state.GraphPath
        if ($LASTEXITCODE -ne 0) {
            throw "Graphify query failed for '$graphKind' (exit code $LASTEXITCODE)."
        }
        $queried++
    }

    if ($queried -eq 0) {
        throw "No requested graph was available."
    }
}

try {
    switch ($Command) {
        "setup" {
            Install-Graphify
        }
        "status" {
            Show-Status
        }
        "build" {
            if ($Target -notin @("canonical", "research", "all")) {
                throw "Usage: graphify_ccl8.cmd build canonical|research|all"
            }
            $buildKinds = if ($Target -eq "all") { @("canonical", "research") } else { @($Target) }
            foreach ($kind in $buildKinds) {
                Build-Graph -Kind $kind
            }
        }
        "query" {
            if ($Target -notin @("canonical", "research", "both")) {
                throw 'Usage: graphify_ccl8.cmd query canonical|research|both "question"'
            }
            $question = ($Remaining -join " ").Trim()
            if ([string]::IsNullOrWhiteSpace($question)) {
                throw "A query is required."
            }
            Query-Graph -Kind $Target -Question $question
        }
    }
}
catch {
    Write-Error $_
    exit 1
}
