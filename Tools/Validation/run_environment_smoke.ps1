param(
    [ValidateSet('Standalone', 'Listen', 'Dedicated')][string]$Mode = 'Standalone',
    [ValidateSet('EnvironmentPlayground', 'EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [int]$Port = 19801,
    [switch]$Rendered,
    [switch]$Restart,
    [switch]$Travel,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
if ($Rendered -and $Mode -ne 'Standalone') { throw 'Rendered inspection runs in Standalone; network modes use NullRHI.' }
if ($Restart -and ($Rendered -or $Mode -ne 'Standalone')) { throw 'Restart is an isolated headless Standalone check.' }
if ($Travel -and ($Restart -or $Rendered -or $Mode -ne 'Standalone' -or $Map -ne 'EnvironmentPlayground')) { throw 'Travel starts in headless Standalone EnvironmentPlayground.' }
$runId = [Guid]::NewGuid().ToString('N')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repository 'CCL.uproject'
$directory = Join-Path $repository ('Saved/Tests/Environment/{0}-{1}-{2}' -f $Map, $Mode, (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$processes = [Collections.Generic.List[object]]::new()
$startedAt = [DateTime]::UtcNow
function Start-Experiment([string]$Label, [string[]]$Options) {
    $log = Join-Path $directory ($Label + '.log')
    $arguments = @(('"{0}"' -f $project)) + $Options + @('-unattended', '-nosplash', '-nosound', '-nop4', '-NoLiveCoding',
        '-CCLExperimentSmoke', ('-CCLExperimentRun=' + $runId), '-ExecCmds="t.MaxFPS 60,r.MotionBlurQuality 0"', ('-abslog="{0}"' -f $log))
    if ($Rendered) {
        $arguments += @('-windowed', '-ForceRes', '-ResX=1280', '-ResY=720', '-CCLExperimentCapture', ('-CCLExperimentCaptureDir="{0}"' -f $directory))
    } else { $arguments += '-nullrhi' }
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log;Label=$Label}
    $processes.Add($entry)
    return $entry
}
function Wait-Experiment($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $text = Get-Content -LiteralPath $Entry.Log -Raw
            if ($text -match 'CCL_EXPERIMENT_SMOKE FAIL') { throw "Experiment assertion failed: $($Entry.Log)" }
            if ($text -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw "Process exited before $Pattern : $($Entry.Log)" }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timeout waiting for $Pattern : $($Entry.Log)"
}
function Check-Exit($Entry) {
    if (-not $Entry.Process.WaitForExit(30000) -or $Entry.Process.ExitCode -ne 0) { throw "Process shutdown failed: $($Entry.Log)" }
}
try {
    $mapPath = '/Game/Maps/' + $Map
    if ($Travel) {
        $traveler = Start-Experiment 'travel' @($mapPath, '-game', '-CCLExperimentTravel')
        Wait-Experiment $traveler 'CCL_EXPERIMENT_SMOKE PASS Role=Travel'
        Check-Exit $traveler
    } elseif ($Restart) {
        $expected = '-CCLExperimentExpected="' + (Join-Path $directory 'expected.bin') + '"'
        $writer = Start-Experiment 'writer' @($mapPath, '-game', '-CCLExperimentCheckpoint=write', $expected)
        Wait-Experiment $writer 'CCL_EXPERIMENT_SMOKE PASS Role=CheckpointWrite'
        Check-Exit $writer
        $reader = Start-Experiment 'reader' @($mapPath, '-game', '-CCLExperimentCheckpoint=read', $expected)
        Wait-Experiment $reader 'CCL_EXPERIMENT_SMOKE PASS Role=CheckpointRead'
        Check-Exit $reader
    } elseif ($Mode -eq 'Standalone') {
        $authority = Start-Experiment 'standalone' @($mapPath, '-game')
    } elseif ($Mode -eq 'Listen') {
        $authority = Start-Experiment 'host' @(($mapPath + '?listen'), '-game', ('-port={0}' -f $Port))
    } else {
        $authority = Start-Experiment 'server' @($mapPath, '-server', ('-port={0}' -f $Port))
    }
    if (-not $Restart -and -not $Travel) {
        Wait-Experiment $authority 'CCL_EXPERIMENT_SMOKE PASS Role=Authority'
        if ($Mode -eq 'Standalone') {
            Check-Exit $authority
        } else {
            Wait-Experiment $authority ('IpNetDriver listening on port ' + $Port)
            if ($Mode -eq 'Dedicated') {
                $driver = Start-Experiment 'driver' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLExperimentRole=driver')
                Wait-Experiment $driver 'CCL_EXPERIMENT_SMOKE PASS Role=Driver'
            }
            $observer = Start-Experiment 'observer' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLExperimentRole=observer')
            Wait-Experiment $observer 'CCL_EXPERIMENT_SMOKE PASS Role=Observer'
            Check-Exit $observer
            if ($authority.Process.HasExited) { throw 'Authority stopped unexpectedly' }
            if ($driver -and $driver.Process.HasExited) { throw 'Driver stopped unexpectedly' }
        }
    }
    if ($Rendered) {
        foreach ($name in @('controls.png', 'station.png', 'overview.png', 'celestial-controls.png', 'shelter-controls.png', 'shelter-half.png', 'shelter-open.png')) {
            $path = Join-Path $directory $name
            if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -lt 1000) { throw "Missing capture $path" }
        }
    }
    [ordered]@{passed=$true;travel=[bool]$Travel;restart=[bool]$Restart;run_id=$runId;map=$Map;mode=$Mode;rendered=[bool]$Rendered;duration_seconds=([DateTime]::UtcNow-$startedAt).TotalSeconds;
        logs=@($processes | ForEach-Object {$_.Log})} | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output "EXPERIMENT PASS $Map $Mode Rendered=$Rendered $directory"
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
