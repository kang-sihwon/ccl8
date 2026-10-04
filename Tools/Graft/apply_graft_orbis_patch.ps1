[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("apply", "check", "revert")]
    [string]$Mode = "apply",

    [string]$GraftRoot = $env:GRAFT_PACKAGE_ROOT
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$script:PatchedFiles = @(
    "dist\ingest\fs.js",
    "dist\graph\generic.js",
    "dist\graph\source-files.js"
)

function Resolve-GraftRoot {
    param([string]$Requested)

    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($Requested)) {
        [void]$candidates.Add($Requested)
    }
    # Prefer this repository's isolated runtime before any global installation.
    [void]$candidates.Add((Join-Path $PSScriptRoot '..\..\.local\graft-runtime\node_modules\@nanonets\graft'))
    if (-not [string]::IsNullOrWhiteSpace($env:APPDATA)) {
        [void]$candidates.Add((Join-Path $env:APPDATA "npm\node_modules\@nanonets\graft"))
    }

    foreach ($candidate in $candidates) {
        $packageFile = Join-Path $candidate "package.json"
        if (Test-Path -LiteralPath $packageFile -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Global @nanonets/graft was not found. Pass -GraftRoot or run 'npm install -g @nanonets/graft@0.18.0'."
}

function Get-PatchState {
    param([string]$Root)

    $marked = @(
        foreach ($relativePath in $script:PatchedFiles) {
            $fullPath = Join-Path $Root $relativePath
            if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
                throw "Expected Graft file was not found: $fullPath"
            }
            [System.IO.File]::ReadAllText($fullPath).Contains("Orbis local patch")
        }
    )

    $count = @($marked | Where-Object { $_ }).Count
    if ($count -eq 0) { return "clean" }
    if ($count -eq $script:PatchedFiles.Count) { return "patched" }
    return "partial"
}

function Invoke-GitApply {
    param(
        [string]$Root,
        [string]$Patch,
        [switch]$Check,
        [switch]$Reverse
    )

    $arguments = [System.Collections.Generic.List[string]]::new()
    [void]$arguments.Add("apply")
    [void]$arguments.Add("--no-index")
    [void]$arguments.Add("--whitespace=nowarn")
    [void]$arguments.Add("--ignore-space-change")
    if ($Check) { [void]$arguments.Add("--check") }
    if ($Reverse) { [void]$arguments.Add("--reverse") }
    [void]$arguments.Add($Patch)

    Push-Location $Root
    try {
        & git.exe @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "git apply failed with exit code $LASTEXITCODE. Graft 0.18.0 may have been modified or reinstalled with different contents."
        }
    }
    finally {
        Pop-Location
    }
}

$resolvedRoot = Resolve-GraftRoot -Requested $GraftRoot
$package = Get-Content -LiteralPath (Join-Path $resolvedRoot "package.json") -Raw | ConvertFrom-Json
if ($package.version -ne "0.18.0") {
    throw "The Orbis patch targets Graft 0.18.0, but $resolvedRoot contains version $($package.version)."
}

$patchFile = Join-Path $PSScriptRoot "graft-orbis-local.patch"
if (-not (Test-Path -LiteralPath $patchFile -PathType Leaf)) {
    throw "Patch file was not found: $patchFile"
}

$state = Get-PatchState -Root $resolvedRoot
if ($state -eq "partial") {
    throw "Graft is partially patched. Reinstall @nanonets/graft@0.18.0 before applying or reverting the Orbis patch."
}

Write-Host "Graft: $resolvedRoot"
Write-Host "Version: $($package.version)"
Write-Host "State: $state"

switch ($Mode) {
    "check" {
        if ($state -eq "patched") {
            Write-Host "Orbis local patch is applied."
            exit 0
        }
        Invoke-GitApply -Root $resolvedRoot -Patch $patchFile -Check
        Write-Host "Orbis local patch can be applied cleanly."
    }
    "apply" {
        if ($state -eq "patched") {
            Write-Host "Orbis local patch is already applied; nothing changed."
            exit 0
        }
        Invoke-GitApply -Root $resolvedRoot -Patch $patchFile -Check
        Invoke-GitApply -Root $resolvedRoot -Patch $patchFile
        if ((Get-PatchState -Root $resolvedRoot) -ne "patched") {
            throw "Patch command completed but the three Orbis markers were not found."
        }
        Write-Host "Orbis local patch applied successfully."
    }
    "revert" {
        if ($state -eq "clean") {
            Write-Host "Orbis local patch is not applied; nothing changed."
            exit 0
        }
        Invoke-GitApply -Root $resolvedRoot -Patch $patchFile -Check -Reverse
        Invoke-GitApply -Root $resolvedRoot -Patch $patchFile -Reverse
        if ((Get-PatchState -Root $resolvedRoot) -ne "clean") {
            throw "Reverse patch completed but Orbis markers remain."
        }
        Write-Host "Orbis local patch reverted successfully."
    }
}
