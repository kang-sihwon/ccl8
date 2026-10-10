param(
    [ValidateSet('Standalone', 'Listen', 'Dedicated')][string]$Mode = 'Standalone',
    [ValidateSet('EnvironmentPlayground', 'EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [int]$Port = 19801,
    [switch]$Rendered,
    [switch]$Restart,
    [switch]$Travel,
    [string]$PackagedExecutable,
    [string]$ServerExecutable,
    [ValidateRange(0, 500)][int]$PacketLagMs = 0,
    [ValidateRange(0, 20)][int]$PacketLossPercent = 0,
    [switch]$Reconnect,
    [ValidateRange(0, 36000)][int]$ProfileFrames = 0,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
if ($Rendered -and $Mode -ne 'Standalone') { throw 'Rendered inspection runs in Standalone; network modes use NullRHI.' }
if ($Restart -and ($Rendered -or $Mode -ne 'Standalone')) { throw 'Restart is an isolated headless Standalone check.' }
if ($Travel -and ($Restart -or $Rendered -or $Mode -ne 'Standalone' -or $Map -ne 'EnvironmentPlayground')) { throw 'Travel starts in headless Standalone EnvironmentPlayground.' }
if ($ProfileFrames -gt 0 -and ($Restart -or $Travel -or $Mode -ne 'Standalone')) { throw 'Frame profiling runs in a separate Standalone process.' }
if ($Reconnect -and $Mode -eq 'Standalone') { throw 'Reconnect requires a network mode.' }
if ($ServerExecutable -and $Mode -ne 'Dedicated') { throw 'ServerExecutable requires Dedicated mode.' }
$runId = [Guid]::NewGuid().ToString('N')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repository 'CCL.uproject'
if ($PackagedExecutable) { $PackagedExecutable = (Resolve-Path -LiteralPath $PackagedExecutable).Path }
if ($ServerExecutable) { $ServerExecutable = (Resolve-Path -LiteralPath $ServerExecutable).Path }
$directory = Join-Path $repository ('Saved/Tests/Environment/{0}-{1}-{2}' -f $Map, $Mode, (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$processes = [Collections.Generic.List[object]]::new()
$startedAt = [DateTime]::UtcNow
$samples = [Collections.Generic.List[object]]::new()
$sampleClock = [Diagnostics.Stopwatch]::StartNew()
$lastSampleMilliseconds = -500.0
$peakConcurrentWorkingSetBytes = 0L
$passed = $false
$failure = $null
$frameProfile = $null
function Measure-Processes {
    if ($sampleClock.Elapsed.TotalMilliseconds - $script:lastSampleMilliseconds -lt 500) { return }
    $script:lastSampleMilliseconds = $sampleClock.Elapsed.TotalMilliseconds
    $concurrentWorkingSetBytes = 0L
    foreach ($entry in $processes) {
        try {
            $entry.Process.Refresh()
            if ($entry.Process.HasExited) { continue }
            $sample = [pscustomobject]@{
                elapsed_seconds = $sampleClock.Elapsed.TotalSeconds
                role = $entry.Label
                process_id = $entry.Process.Id
                cpu_seconds = $entry.Process.TotalProcessorTime.TotalSeconds
                working_set_bytes = $entry.Process.WorkingSet64
                private_bytes = $entry.Process.PrivateMemorySize64
                peak_working_set_bytes = $entry.Process.PeakWorkingSet64
            }
            $samples.Add($sample)
            $concurrentWorkingSetBytes += $sample.working_set_bytes
        } catch [InvalidOperationException] {
            # A process can exit between Refresh and reading its counters.
        }
    }
    $script:peakConcurrentWorkingSetBytes = [Math]::Max($script:peakConcurrentWorkingSetBytes, $concurrentWorkingSetBytes)
}
function Start-Experiment([string]$Label, [string[]]$Options) {
    $log = Join-Path $directory ($Label + '.log')
    $binary = $editor
    $prefix = @(('"{0}"' -f $project))
    if ($ServerExecutable -and $Options -contains '-server') { $binary = $ServerExecutable; $prefix = @() }
    elseif ($PackagedExecutable) { $binary = $PackagedExecutable; $prefix = @() }
    $arguments = $prefix + $Options + @('-unattended', '-nosplash', '-nosound', '-nop4', '-NoLiveCoding',
        ('-abslog="{0}"' -f $log))
    if ($ProfileFrames -gt 0) {
        $arguments += @('-ExitAfterCsvProfiling', '-csvCompression=0', ('-UserDir="{0}"' -f (Join-Path $directory 'profile')),
            ('-ExecCmds="t.MaxFPS 60,r.MotionBlurQuality 0,csvprofile STARTFILE=frames,csvprofile FRAMES={0}"' -f $ProfileFrames))
        if ($Rendered) { $arguments += '-csvGpuStats' }
    } else {
        $arguments += @('-CCLExperimentSmoke', ('-CCLExperimentRun=' + $runId), '-ExecCmds="t.MaxFPS 60,r.MotionBlurQuality 0"')
    }
    $arguments += @(('-PktLag=' + $PacketLagMs), ('-PktLoss=' + $PacketLossPercent))
    if ($Rendered) {
        $arguments += @('-windowed', '-ForceRes', '-ResX=1280', '-ResY=720', '-CCLExperimentCapture', ('-CCLExperimentCaptureDir="{0}"' -f $directory))
    } else { $arguments += '-nullrhi' }
    $process = Start-Process -FilePath $binary -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log;Label=$Label}
    $processes.Add($entry)
    return $entry
}
function Wait-Experiment($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Measure-Processes
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
    if ($ProfileFrames -gt 0) {
        $profile = Start-Experiment 'profile' @($mapPath, '-game')
        Wait-Experiment $profile 'Capture Ended\. Writing CSV to file'
        Check-Exit $profile
        $csvPath = Join-Path $directory 'profile/Saved/Profiling/CSV/frames.csv'
        if (-not (Test-Path -LiteralPath $csvPath) -or (Get-Item -LiteralPath $csvPath).Length -lt 100) { throw "Missing frame profile: $csvPath" }
        $python = Join-Path $paths.engineRoot 'Binaries/ThirdParty/Python3/Win64/python.exe'
        $profileArguments = @('-B', (Join-Path $PSScriptRoot 'summarize_environment_profile.py'), '--csv', $csvPath,
            '--output', (Join-Path $directory 'frame-summary.json'), '--expected-frames', $ProfileFrames,
            '--warmup-frames', [Math]::Min(30, $ProfileFrames - 1))
        if (-not $Rendered) { $profileArguments += '--null-rhi' }
        & $python @profileArguments
        if ($LASTEXITCODE -ne 0) { throw 'Frame profile validation failed' }
        $frameProfile = $csvPath
    } elseif ($Travel) {
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
    if ($ProfileFrames -eq 0 -and -not $Restart -and -not $Travel) {
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
            if ($Reconnect) {
                $reconnected = Start-Experiment 'observer-reconnect' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLExperimentRole=observer')
                Wait-Experiment $reconnected 'CCL_EXPERIMENT_SMOKE PASS Role=Observer'
                Check-Exit $reconnected
            }
            if ($authority.Process.HasExited) { throw 'Authority stopped unexpectedly' }
            if ($driver -and $driver.Process.HasExited) { throw 'Driver stopped unexpectedly' }
        }
    }
    if ($Rendered -and $ProfileFrames -eq 0) {
        foreach ($name in @('controls.png', 'station.png', 'overview.png', 'celestial-controls.png', 'shelter-controls.png', 'shelter-half.png', 'shelter-open.png')) {
            $path = Join-Path $directory $name
            if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -lt 1000) { throw "Missing capture $path" }
        }
    }
    if ($Mode -ne 'Standalone') {
        foreach ($entry in $processes) {
            $logText = Get-Content -LiteralPath $entry.Log -Raw
            if ($PacketLagMs -gt 0 -and $logText -notmatch ('PktLag set to ' + $PacketLagMs + '\b')) { throw "Lag setting was not applied: $($entry.Log)" }
            if ($PacketLossPercent -gt 0 -and $logText -notmatch ('PktLoss set to ' + $PacketLossPercent + '\b')) { throw "Loss setting was not applied: $($entry.Log)" }
        }
    }
    $passed = $true
    if ($ProfileFrames -gt 0) { Write-Output "PROFILE PASS $Map Rendered=$Rendered Frames=$ProfileFrames $directory" }
    else { Write-Output "EXPERIMENT PASS $Map $Mode Rendered=$Rendered $directory" }
} catch {
    $failure = $_.Exception.Message
    throw
} finally {
    $metrics = @($processes | ForEach-Object {
        $entry = $_
        $observed = @($samples | Where-Object process_id -eq $entry.Process.Id)
        [ordered]@{
            role = $entry.Label
            process_id = $entry.Process.Id
            measured = ($observed.Count -gt 0)
            samples = $observed.Count
            last_observed_cpu_seconds = $(if ($observed.Count) { $observed[-1].cpu_seconds } else { $null })
            peak_observed_working_set_bytes = $(if ($observed.Count) { ($observed | Measure-Object working_set_bytes -Maximum).Maximum } else { $null })
            peak_observed_private_bytes = $(if ($observed.Count) { ($observed | Measure-Object private_bytes -Maximum).Maximum } else { $null })
            last_os_peak_working_set_bytes = $(if ($observed.Count) { $observed[-1].peak_working_set_bytes } else { $null })
        }
    })
    $networkMetrics = @($processes | ForEach-Object {
        $entry = $_
        if (Test-Path -LiteralPath $entry.Log) {
            foreach ($line in [IO.File]::ReadLines($entry.Log)) {
                if ($line -match 'CCL_EXPERIMENT_NET Role=(\w+) Seconds=([0-9.]+) InBytes=(\d+) OutBytes=(\d+) InPackets=(\d+) OutPackets=(\d+)') {
                    $seconds = [double]::Parse($Matches[2], [Globalization.CultureInfo]::InvariantCulture)
                    $received = [long]$Matches[3]; $sent = [long]$Matches[4]
                    [ordered]@{
                        process_role=$entry.Label; completed_role=$Matches[1]; connection_seconds=$seconds
                        received_bytes=$received; sent_bytes=$sent; received_packets=[long]$Matches[5]; sent_packets=[long]$Matches[6]
                        average_received_bytes_per_second=$(if ($seconds -gt 0) { $received / $seconds } else { $null })
                        average_sent_bytes_per_second=$(if ($seconds -gt 0) { $sent / $seconds } else { $null })
                    }
                }
            }
        }
    })
    try {
        $samples | Export-Csv -LiteralPath (Join-Path $directory 'process-samples.csv') -NoTypeInformation -Encoding UTF8
        [ordered]@{
            run_kind=$(if ($ProfileFrames -gt 0) { 'frame_profile' } else { 'functional_smoke' })
            passed=$passed; failure=$failure; server_target=[bool]$ServerExecutable; packaged_client=[bool]$PackagedExecutable
            packet_lag_ms=$PacketLagMs; packet_loss_percent=$PacketLossPercent; observer_reconnect=[bool]$Reconnect
            travel=[bool]$Travel; restart=[bool]$Restart; run_id=$runId; map=$Map; mode=$Mode; rendered=[bool]$Rendered
            duration_seconds=([DateTime]::UtcNow-$startedAt).TotalSeconds
            peak_observed_concurrent_working_set_bytes=$peakConcurrentWorkingSetBytes
            profile_frames_requested=$ProfileFrames; frame_profile=$frameProfile
            process_metrics=$metrics; network_metrics=$networkMetrics
            logs=@($processes | ForEach-Object {$_.Log})
        } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
    } finally {
        foreach ($entry in $processes) {
            if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
        }
    }
}
