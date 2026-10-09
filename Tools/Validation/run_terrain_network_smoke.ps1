param(
    [ValidateSet('Listen','Dedicated')][string]$Mode = 'Dedicated',
    [int]$Port = 19831,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/TerrainNetwork/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$processes = [Collections.Generic.List[object]]::new()
function Start-Terrain([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NullRHI','-NoLiveCoding','-CCLTerrainNetworkSmoke','-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Terrain($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $text = Get-Content -LiteralPath $Entry.Log -Raw
            if ($text -match 'CCL_TERRAIN_NETWORK FAIL') { throw ('Terrain network assertion: '+$Entry.Log) }
            if ($text -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Terrain process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Terrain network timeout: '+$Entry.Log)
}
try {
    $map='/Game/Maps/EnvironmentScenario'
    $options=if ($Mode -eq 'Listen') { @($map+'?listen','-game',('-port='+$Port)) } else { @($map,'-server',('-port='+$Port)) }
    $server=Start-Terrain 'server' $options
    Wait-Terrain $server 'CCL_TERRAIN_NETWORK BASE_READY'
    $first=Start-Terrain 'first' @(('127.0.0.1:'+$Port),'-game')
    Wait-Terrain $server 'CCL_TERRAIN_NETWORK EDIT_READY'
    $late=Start-Terrain 'late' @(('127.0.0.1:'+$Port),'-game')
    foreach ($entry in @($first,$late,$server)) {
        Wait-Terrain $entry 'CCL_TERRAIN_NETWORK PASS'
        if (-not $entry.Process.WaitForExit(30000) -or $entry.Process.ExitCode -ne 0) { throw ('Shutdown failed: '+$entry.Log) }
    }
    $serverText=Get-Content $server.Log -Raw
    if ($serverText -notmatch 'CCL_TERRAIN_TRANSFER Serial=2 Base=1' -or $serverText -notmatch 'CCL_TERRAIN_TRANSFER Serial=2 Base=0') { throw 'Snapshot/delta/late-join transfer evidence missing' }
    if ($serverText -notmatch 'DROPPED_READY_ACK Serial=2' -or $serverText -notmatch 'TRANSFER_TIMEOUT Serial=2') { throw 'ACK timeout/retry evidence missing' }
    [ordered]@{passed=$true;mode=$Mode;clients=2;late_join=$true;snapshot_delta=$true;ready_ack_retry=$true} | ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('TERRAIN_NETWORK PASS '+$directory)
} finally {
    foreach ($entry in $processes) { if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue } }
}
