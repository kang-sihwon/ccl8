param(
    [ValidateSet('Standalone', 'Dedicated', 'Listen')]
    [string]$Mode = 'Standalone',
    [int]$Port = 18777,
    [switch]$DriverIsHost,
    [switch]$Impaired
)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repository 'CCL.uproject'
$map = '/Game/Maps/CombatPlayground'
$logDirectory = Join-Path $repository ('Saved/Tests/CombatSmoke/{0}-{1}' -f $Mode, (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$started = [Collections.Generic.List[System.Diagnostics.Process]]::new()
$checks = [Collections.Generic.List[string]]::new()

function Start-Game([string]$Label, [string[]]$Options) {
    $log = Join-Path $logDirectory ($Label + '.log')
    $arguments = @(('"{0}"' -f $project)) + $Options + @(
        '-unattended', '-nosplash', '-nosound', '-nullrhi', '-NoScreenMessages',
        '-DisableAllScreenMessages', '-NoLiveCoding', '-NoAsyncLoadingThread',
        ('-ExecCmds="t.MaxFPS 60' + $(if ($Impaired) { ',NetEmulation.PktLag 50,NetEmulation.PktLoss 2' } else { '' }) + '"'), ('-abslog="{0}"' -f $log)
    )
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $started.Add($process)
    return $log
}

try {
    if ($Mode -eq 'Standalone') {
        $checks.Add((Start-Game 'driver' @($map, '-game', '-CCLCombatSmoke=driver', '-CCLSmokePlayers=1')))
    } else {
        if ($Mode -eq 'Dedicated') {
            $serverLog = Start-Game 'server' @($map, '-server', '-CCLCombatSmoke=server', ('-port={0}' -f $Port))
        } else {
            $hostRole = if ($DriverIsHost) { 'driver' } else { 'witness' }
            $serverLog = Start-Game 'host' @(($map + '?listen'), '-game', ('-port={0}' -f $Port), ('-CCLCombatSmoke={0}' -f $hostRole), '-CCLSmokePlayers=2')
            $checks.Add($serverLog)
        }
        if ($Mode -eq 'Dedicated') { $checks.Add($serverLog) }
        $deadline = [DateTime]::UtcNow.AddSeconds(60)
        do {
            if ((Test-Path -LiteralPath $serverLog) -and
                (Select-String -LiteralPath $serverLog -Pattern ('IpNetDriver listening on port ' + $Port) -Quiet)) { break }
            if ([DateTime]::UtcNow -gt $deadline) { throw 'Server did not listen within 60 seconds' }
            Start-Sleep -Milliseconds 500
        } while ($true)
        if ($Mode -eq 'Dedicated') {
            ((Start-Game 'witness' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLCombatSmoke=witness', '-CCLSmokePlayers=2')))
        }
        $clientRole = if ($Mode -eq 'Listen' -and $DriverIsHost) { 'witness' } else { 'driver' }
        ((Start-Game $clientRole @(('127.0.0.1:{0}' -f $Port), '-game', ('-CCLCombatSmoke={0}' -f $clientRole), '-CCLSmokePlayers=2')))
    }

    $deadline = [DateTime]::UtcNow.AddSeconds(180)
    do {
        Get-ChildItem -LiteralPath $logDirectory -Filter '*.log' | ForEach-Object {
            if (Select-String -LiteralPath $_.FullName -Pattern 'CCL_COMBAT FAIL|Fatal error:|Assertion failed:' -Quiet) {
                throw ('Combat failure: ' + $_.FullName)
            }
        }
        $passed = 0
        foreach ($log in $checks) {
            if (!(Test-Path -LiteralPath $log)) { continue }
            if (Select-String -LiteralPath $log -Pattern 'CCL_COMBAT FAIL|Fatal error:|Assertion failed:' -Quiet) {
                throw ('Smoke failure: ' + $log)
            }
            if (Select-String -LiteralPath $log -Pattern 'CCL_COMBAT PASS' -Quiet) { $passed++ }
        }
        if ($passed -eq $checks.Count) { break }
        if ([DateTime]::UtcNow -gt $deadline) { throw 'Smoke test timed out' }
        Start-Sleep -Milliseconds 500
    } while ($true)
    Write-Output ('PASS ' + $Mode)
    foreach ($log in $checks) {
        Select-String -LiteralPath $log -Pattern 'CCL_COMBAT' | ForEach-Object { $_.Line }
    }
} finally {
    foreach ($process in $started) {
        if (!$process.HasExited) { $process.Kill() }
        $process.Dispose()
    }
    Write-Output ('Logs: ' + $logDirectory)
}
