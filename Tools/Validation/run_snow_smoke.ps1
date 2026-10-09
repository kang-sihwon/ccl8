param(
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [switch]$Rendered,
    [switch]$Correction,
    [int]$Port = 19871,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
if ($Correction -and $Mode -ne 'Dedicated') { throw 'History expiry correction uses a dedicated server and an autonomous client' }
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/Snow/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-Snow([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding','-CCLSnowSmoke',('-CCLSnowRole='+$Name),
        ('-CCLSnowSmokeDir="'+$directory+'"'),('-CCLExperimentRun='+$runId),('-ExecCmds="t.MaxFPS 60' + $(if ($Correction) { ',NetEmulation.PktLag 50,NetEmulation.PktLoss 2' } else { '' }) + '"'),('-abslog="'+$log+'"'))
    if ($Correction) { $args += '-CCLSnowCorrection' }
    if ($Rendered -and $Mode -eq 'Standalone') { $args += @('-CCLSnowCapture','-windowed','-ForceRes','-ResX=1280','-ResY=720') }
    else { $args += '-NullRHI' }
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Snow($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_SNOW_SMOKE FAIL') { throw ('Snow assertion: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Snow process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Snow timeout: '+$Entry.Log)
}
try {
    $map = '/Game/Maps/EnvironmentScenario'
    $options = if ($Mode -eq 'Dedicated') { @($map,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($map+'?listen','-game',('-port='+$Port)) }
        else { @($map,'-game') }
    $server = Start-Snow 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-Snow $server 'CCL_SNOW_SMOKE BASE_READY'
        $first = Start-Snow 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-Snow $server 'CCL_SNOW_SMOKE RESTORE_READY'
        $late = Start-Snow 'late' @(('127.0.0.1:'+$Port),'-game')
        Wait-Snow $first 'CCL_SNOW_SMOKE PASS'
        Wait-Snow $late 'CCL_SNOW_SMOKE PASS'
    }
    Wait-Snow $server 'CCL_SNOW_SMOKE PASS'
    if ($Mode -eq 'Standalone') {
        $shutdownDeadline = [DateTime]::UtcNow.AddSeconds(120)
        while (-not $server.Process.HasExited -and [DateTime]::UtcNow -lt $shutdownDeadline) { Start-Sleep -Milliseconds 500 }
        if (-not $server.Process.HasExited -or $server.Process.ExitCode -ne 0) { throw ('Snow shutdown failed: '+$server.Log) }
    }
    if ($Rendered) {
        foreach ($name in @('snow-controls.png','snow-walk.png','snow-feet.png','snow-trail.png')) {
            $path = Join-Path $directory $name
            if (-not (Test-Path $path) -or (Get-Item $path).Length -lt 1000) { throw ('Missing screenshot '+$path) }
        }
    }
    if ($Mode -ne 'Standalone') {
        $expected = (Get-FileHash -LiteralPath (Join-Path $directory 'server-surface.bin')).Hash
        foreach ($peer in @('first','late')) {
            if ((Get-FileHash -LiteralPath (Join-Path $directory ($peer+'-surface.bin'))).Hash -ne $expected) {
                throw ('Canonical snow snapshot mismatch: '+$peer)
            }
        }
    }
    [ordered]@{passed=$true;correction=[bool]$Correction;mode=$Mode;rendered=[bool]$Rendered;clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone');run_id=$runId} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('SNOW_SMOKE PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
