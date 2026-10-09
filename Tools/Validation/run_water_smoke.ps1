param(
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [switch]$Rendered,
    [int]$Port = 19861,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/Water/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-Water([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding','-CCLWaterSmoke',
        ('-CCLWaterSmokeDir="'+$directory+'"'),('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    if ($Rendered -and $Mode -eq 'Standalone') { $args += @('-CCLWaterCapture','-windowed','-ForceRes','-ResX=1280','-ResY=720') }
    else { $args += '-NullRHI' }
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Water($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_WATER_SMOKE FAIL') { throw ('Water assertion: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Water process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Water timeout: '+$Entry.Log)
}
try {
    $map = '/Game/Maps/EnvironmentScenario'
    $options = if ($Mode -eq 'Dedicated') { @($map,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($map+'?listen','-game',('-port='+$Port)) }
        else { @($map,'-game') }
    $server = Start-Water 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-Water $server 'CCL_WATER_SMOKE BASE_READY'
        $first = Start-Water 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-Water $server 'CCL_WATER_SMOKE RESTORE_READY'
        $late = Start-Water 'late' @(('127.0.0.1:'+$Port),'-game')
        Wait-Water $first 'CCL_WATER_SMOKE PASS'
        Wait-Water $late 'CCL_WATER_SMOKE PASS'
    }
    Wait-Water $server 'CCL_WATER_SMOKE PASS'
    if ($Mode -eq 'Standalone') {
        $shutdownDeadline = [DateTime]::UtcNow.AddSeconds(120)
        while (-not $server.Process.HasExited -and [DateTime]::UtcNow -lt $shutdownDeadline) { Start-Sleep -Milliseconds 500 }
        if (-not $server.Process.HasExited -or $server.Process.ExitCode -ne 0) { throw ('Water shutdown failed: '+$server.Log) }
    }
    if ($Rendered) {
        foreach ($name in @('water-controls.png','water-overview.png','water-ice.png','water-mud.png')) {
            $path = Join-Path $directory $name
            if (-not (Test-Path $path) -or (Get-Item $path).Length -lt 1000) { throw ('Missing screenshot '+$path) }
        }
    }
    [ordered]@{passed=$true;mode=$Mode;rendered=[bool]$Rendered;clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone');run_id=$runId} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('WATER_SMOKE PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
