param(
    [ValidateSet('EnvironmentPlayground','EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [int]$Port = 20321,
    [ValidateRange(0,500)][int]$PacketLagMs = 0,
    [ValidateRange(0,20)][int]$PacketLossPercent = 0,
    [string]$PackagedExecutable,
    [string]$ServerExecutable,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
if ($ServerExecutable -and $Mode -ne 'Dedicated') { throw 'ServerExecutable requires Dedicated mode.' }
if ($PackagedExecutable) { $PackagedExecutable = (Resolve-Path -LiteralPath $PackagedExecutable).Path }
if ($ServerExecutable) { $ServerExecutable = (Resolve-Path -LiteralPath $ServerExecutable).Path }
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/EcologySpace/{0}-{1}-{2}' -f $Map,$Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-Space([string]$Label, [string[]]$Options) {
    $log = Join-Path $directory ($Label+'.log')
    $binary = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
    $prefix = @(('"'+$root+'/CCL.uproject"'))
    if ($ServerExecutable -and $Options -contains '-server') { $binary = $ServerExecutable; $prefix = @() }
    elseif ($PackagedExecutable) { $binary = $PackagedExecutable; $prefix = @() }
    $arguments = $prefix + $Options + @('-unattended','-nop4','-nosplash','-nosound',
        '-NullRHI','-NoLiveCoding','-CCLEcologySpaceSmoke',('-CCLEcologySpaceDir="'+$directory+'"'),
        ('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $arguments += @('-PktLag='+$PacketLagMs, '-PktLoss='+$PacketLossPercent)
    $process = Start-Process -FilePath $binary -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Space($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_ECOLOGY_SPACE FAIL|Fatal error:|Assertion failed:') { throw ('Ecology space failure: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Ecology space exited before verification: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Ecology space timeout: '+$Entry.Log)
}
function Wait-SpaceExit($Entry) {
    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    while (-not $Entry.Process.HasExited -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 500 }
    if (-not $Entry.Process.HasExited -or $Entry.Process.ExitCode -ne 0) { throw ('Ecology space shutdown failed: '+$Entry.Log) }
}
try {
    $mapPath = '/Game/Maps/'+$Map
    $options = if ($Mode -eq 'Dedicated') { @($mapPath,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($mapPath+'?listen','-game',('-port='+$Port)) }
        else { @($mapPath,'-game') }
    $server = Start-Space 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-Space $server 'CCL_ECOLOGY_SPACE BASE_READY'
        $first = Start-Space 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-Space $first 'CCL_ECOLOGY_SPACE BASE_OBSERVED'
        'First client observed the initial Actors.' | Set-Content (Join-Path $directory 'first-ready.txt')
        Wait-Space $server 'CCL_ECOLOGY_SPACE RESTORE_READY'
        $late = Start-Space 'late' @(('127.0.0.1:'+$Port),'-game')
        Wait-Space $first 'CCL_ECOLOGY_SPACE PASS'
        Wait-Space $late 'CCL_ECOLOGY_SPACE PASS'
        'Both clients observed restored spatial poses.' | Set-Content (Join-Path $directory 'clients-ready.txt')
        Wait-SpaceExit $first
        Wait-SpaceExit $late
    }
    Wait-Space $server 'CCL_ECOLOGY_SPACE PASS'
    if ($Mode -eq 'Standalone') {
        Wait-SpaceExit $server
        $reader = Start-Space 'reader' @($mapPath,'-game','-CCLEcologySpaceRead')
        Wait-Space $reader 'CCL_ECOLOGY_SPACE CONTINUATION_MATCH'
        Wait-Space $reader 'CCL_ECOLOGY_SPACE PASS'
        Wait-SpaceExit $reader
    }
    if ($Mode -ne 'Standalone') {
        foreach ($entry in $processes) {
            $text = Get-Content -LiteralPath $entry.Log -Raw
            if ($PacketLagMs -gt 0 -and $text -notmatch ('PktLag set to ' + $PacketLagMs + '\b')) { throw ('Lag setting not applied: ' + $entry.Log) }
            if ($PacketLossPercent -gt 0 -and $text -notmatch ('PktLoss set to ' + $PacketLossPercent + '\b')) { throw ('Loss setting not applied: ' + $entry.Log) }
        }
    }
    [ordered]@{passed=$true;packaged_executable=$PackagedExecutable;server_executable=$ServerExecutable;packet_lag_ms=$PacketLagMs;packet_loss_percent=$PacketLossPercent;map=$Map;mode=$Mode;ground_navigation=$true;air_detour=$true;liquid_volume=$true;save_restore=$true;food_history=$true;continuation=($Mode -eq 'Standalone');restart=($Mode -eq 'Standalone');clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone')} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('ECOLOGY_SPACE PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
