param(
    [ValidateSet('Standalone', 'Listen', 'Dedicated')]
    [string]$Mode = 'Standalone',
    [int]$Port = 19781,
    [ValidateRange(30, 600)]
    [int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repository 'CCL.uproject'
$directory = Join-Path $repository ('Saved/Tests/WorldSmoke/{0}-{1}' -f $Mode, (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$processes = [Collections.Generic.List[object]]::new()
$startedAt = [DateTime]::UtcNow
function Start-World([string]$Label, [string[]]$Options) {
    $log = Join-Path $directory ($Label + '.log')
    $arguments = @(('"{0}"' -f $project)) + $Options + @(
        '-unattended', '-nosplash', '-nosound', '-nullrhi', '-nop4', '-NoLiveCoding',
        '-CCLWorldSmoke', '-ExecCmds="t.MaxFPS 60"', ('-abslog="{0}"' -f $log)
    )
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{ Process=$process; Log=$log; Label=$Label }
    $processes.Add($entry)
    return $entry
}
function Wait-Log($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_WORLD_SMOKE FAIL') { throw "World smoke failed: $($Entry.Log)" }
            $match = [regex]::Match($content, $Pattern)
            if ($match.Success) { return $match }
        }
        if ($Entry.Process.HasExited) { throw "Process exited before expected result: $($Entry.Log)" }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timeout waiting for $Pattern in $($Entry.Log)"
}
function Check-Exit($Entry) {
    if (-not $Entry.Process.WaitForExit(30000)) { throw "Process did not exit after PASS: $($Entry.Log)" }
    if ($Entry.Process.ExitCode -ne 0) { throw "Process exit code $($Entry.Process.ExitCode): $($Entry.Log)" }
}
try {
    $map = '/Game/Maps/Campaign'
    if ($Mode -eq 'Standalone') {
        $authority = Start-World 'standalone' @($map, '-game')
    } elseif ($Mode -eq 'Listen') {
        $authority = Start-World 'host' @(($map + '?listen'), '-game', ('-port={0}' -f $Port))
    } else {
        $authority = Start-World 'server' @($map, '-server', ('-port={0}' -f $Port))
    }
    $ready = Wait-Log $authority 'CCL_WORLD_SMOKE READY World=([A-Fa-f0-9]+) Time=([0-9.]+)'
    $null = Wait-Log $authority 'CCL_WORLD_SMOKE PASS Authority'
    if ($Mode -eq 'Standalone') {
        Check-Exit $authority
    } else {
        $null = Wait-Log $authority ('IpNetDriver listening on port ' + $Port)
        $clientCount = if ($Mode -eq 'Dedicated') { 2 } else { 1 }
        for ($index = 1; $index -le $clientCount; ++$index) {
            $client = Start-World ('client-' + $index) @(
                ('127.0.0.1:{0}' -f $Port), '-game',
                ('-CCLWorldId={0}' -f $ready.Groups[1].Value),
                ('-CCLWorldTime={0}' -f $ready.Groups[2].Value)
            )
            $null = Wait-Log $client 'CCL_WORLD_SMOKE PASS Client'
            Check-Exit $client
        }
        if ($authority.Process.HasExited) { throw 'Authority exited unexpectedly' }
    }
    [ordered]@{mode=$Mode; passed=$true; world_id=$ready.Groups[1].Value; world_seconds=$ready.Groups[2].Value;
        duration_seconds=([DateTime]::UtcNow-$startedAt).TotalSeconds; logs=@($processes | ForEach-Object {$_.Log})} |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output "WORLD_SMOKE PASS $Mode $directory"
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
