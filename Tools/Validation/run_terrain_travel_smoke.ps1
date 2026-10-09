param(
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [int]$Port = 19841,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/TerrainTravel/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$processes = [Collections.Generic.List[object]]::new()
function Start-TerrainTravel([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NullRHI','-NoLiveCoding','-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-TerrainTravel($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $text = Get-Content -LiteralPath $Entry.Log -Raw
            if ($text -match 'CCL_TERRAIN_TRAVEL FAIL') { throw ('Terrain travel assertion: '+$Entry.Log) }
            if ($text -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Terrain travel process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Terrain travel timeout: '+$Entry.Log)
}
try {
    $map = '/Game/Maps/EnvironmentScenario'
    $options = @()
    switch ($Mode) {
        'Listen' { $options = @(($map+'?listen'),'-game',('-port='+$Port)) }
        'Dedicated' { $options = @($map,'-server',('-port='+$Port)) }
        default { $options = @($map,'-game') }
    }
    $server = Start-TerrainTravel 'server' ($options + @('-CCLTerrainTravelSmoke'))
    Wait-TerrainTravel $server 'CCL_TERRAIN_TRAVEL BASE_READY'
    if ($Mode -ne 'Standalone') {
        $client = Start-TerrainTravel 'client' @(('127.0.0.1:'+$Port),'-game')
    }
    Wait-TerrainTravel $server 'CCL_TERRAIN_TRAVEL PASS'
    if (-not $server.Process.WaitForExit(30000) -or $server.Process.ExitCode -ne 0) { throw ('Travel shutdown failed: '+$server.Log) }
    [ordered]@{passed=$true;mode=$Mode;travels=4;remote_clients=([int]($Mode -ne 'Standalone'));world_clock_life_terrain=$true;pending_travel_rejected=$true} | ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('TERRAIN_TRAVEL PASS '+$directory)
} finally {
    foreach ($entry in $processes) { if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue } }
}
