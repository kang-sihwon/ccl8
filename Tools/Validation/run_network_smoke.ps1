param(
    [string]$PackagedExecutable,
    [ValidateSet('Standalone', 'Dedicated', 'Listen')]
    [string]$Mode = 'Standalone',
    [int]$Port = 18777,
    [switch]$DriverIsHost,
    [ValidateRange(60, 1800)]
    [int]$StartupTimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if ($PackagedExecutable) {
    if ($Mode -eq 'Dedicated') { throw 'This packaged Game target test supports Standalone and Listen only' }
    $editor = (Resolve-Path -LiteralPath $PackagedExecutable).Path
} else {
    $paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
    $editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
}
$project = Join-Path $repository 'CCL.uproject'
$map = '/Game/Maps/MultiplayerPlayground'
$logDirectory = Join-Path $repository ('Saved/Tests/NetworkSmoke/{0}-{1}' -f $Mode, (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$started = [Collections.Generic.List[System.Diagnostics.Process]]::new()
$checks = [Collections.Generic.List[string]]::new()

function Start-Game([string]$Label, [string[]]$Options) {
    $log = Join-Path $logDirectory ($Label + '.log')
    [string[]]$prefix = if ($PackagedExecutable) { @() } else { @(('"{0}"' -f $project)) }
    $arguments = $prefix + $Options + @(
        '-unattended', '-nosplash', '-nosound', '-nullrhi', '-NoScreenMessages',
        '-DisableAllScreenMessages', '-NoLiveCoding', '-NoAsyncLoadingThread',
        '-ExecCmds="t.MaxFPS 60"', ('-abslog="{0}"' -f $log)
    )
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $started.Add($process)
    return $log
}

try {
    if ($Mode -eq 'Standalone') {
        $checks.Add((Start-Game 'driver' @($map, '-game', '-CCLSmoke=driver', '-CCLSmokePlayers=1')))
    } else {
        if ($Mode -eq 'Dedicated') {
            $serverLog = Start-Game 'server' @($map, '-server', ('-port={0}' -f $Port))
        } else {
            $hostRole = if ($DriverIsHost) { 'driver' } else { 'witness' }
            $serverLog = Start-Game 'host' @(($map + '?listen'), '-game', ('-port={0}' -f $Port), ('-CCLSmoke={0}' -f $hostRole), '-CCLSmokePlayers=2')
            $checks.Add($serverLog)
        }
        $deadline = [DateTime]::UtcNow.AddSeconds($StartupTimeoutSeconds)
        do {
            if ((Test-Path -LiteralPath $serverLog) -and
                (Select-String -LiteralPath $serverLog -Pattern ('IpNetDriver listening on port ' + $Port) -Quiet)) { break }
            if ($started[0].HasExited) { throw ('Server exited before listening: ' + $serverLog) }
            if ([DateTime]::UtcNow -gt $deadline) { throw ('Server startup timed out: ' + $serverLog) }
            Start-Sleep -Milliseconds 500
        } while ($true)
        if ($Mode -eq 'Dedicated') {
            $checks.Add((Start-Game 'witness' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLSmoke=witness', '-CCLSmokePlayers=2')))
        }
        $clientRole = if ($Mode -eq 'Listen' -and $DriverIsHost) { 'witness' } else { 'driver' }
        $checks.Add((Start-Game $clientRole @(('127.0.0.1:{0}' -f $Port), '-game', ('-CCLSmoke={0}' -f $clientRole), '-CCLSmokePlayers=2')))
    }

    $deadline = [DateTime]::UtcNow.AddSeconds($StartupTimeoutSeconds + 90)
    do {
        $passed = 0
        foreach ($log in $checks) {
            if (!(Test-Path -LiteralPath $log)) { continue }
            if (Select-String -LiteralPath $log -Pattern 'CCL_SMOKE FAIL|Fatal error:|Assertion failed:' -Quiet) {
                throw ('Smoke failure: ' + $log)
            }
            if (Select-String -LiteralPath $log -Pattern 'CCL_SMOKE PASS' -Quiet) { $passed++ }
        }
        if ($passed -eq $checks.Count) { break }
        foreach ($process in $started) {
            if ($process.HasExited) { throw ('Game process exited before test completion: ' + $process.Id) }
        }
        if ([DateTime]::UtcNow -gt $deadline) { throw 'Smoke test timed out' }
        Start-Sleep -Milliseconds 500
    } while ($true)
    Write-Output ('PASS ' + $Mode)
    foreach ($log in $checks) {
        Select-String -LiteralPath $log -Pattern 'CCL_SMOKE' | ForEach-Object { $_.Line }
    }
} finally {
    foreach ($process in $started) {
        if (!$process.HasExited) { $process.Kill() }
        $process.Dispose()
    }
    Write-Output ('Logs: ' + $logDirectory)
}
