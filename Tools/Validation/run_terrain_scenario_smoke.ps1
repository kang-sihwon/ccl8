param(
    [ValidateSet('EnvironmentScenario','EnvironmentPlayground')][string]$Map = 'EnvironmentScenario',
    [switch]$Rendered,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths=Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory=Join-Path $root ('Saved/Tests/TerrainScenario/{0}-{1}' -f $Map,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId=[Guid]::NewGuid().ToString('N')
$processes=[Collections.Generic.List[object]]::new()
try {
    foreach ($mode in @('write','read')) {
        $log=Join-Path $directory ($mode+'.log')
        $args=@(('"'+$root+'/CCL.uproject"'),('/Game/Maps/'+$Map),'-game','-unattended','-nop4','-nosound','-nosplash','-NoLiveCoding',
            '-CCLTerrainScenarioSmoke',('-CCLTerrainScenarioMode='+$mode),('-CCLTerrainScenarioDir="'+$directory+'"'),('-CCLExperimentRun='+$runId),
            '-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
        if ($Rendered -and $mode -eq 'write') { $args+=@('-CCLTerrainScenarioCapture','-windowed','-ForceRes','-ResX=1280','-ResY=720') }
        else { $args+='-NullRHI' }
        $process=Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
        $processes.Add($process)
        $deadline=[DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $passed=$false
        do {
            if (Test-Path -LiteralPath $log) {
                $text=Get-Content -LiteralPath $log -Raw
                if ($text -match 'CCL_TERRAIN_SCENARIO FAIL') { throw ('Terrain scenario assertion: '+$log) }
                if ($text -match 'CCL_TERRAIN_SCENARIO PASS') { $passed=$true; break }
            }
            if ($process.HasExited) { throw ('Terrain scenario exited early: '+$log) }
            Start-Sleep -Milliseconds 500
        } while ([DateTime]::UtcNow -lt $deadline)
        if (-not $passed) { throw ('Terrain scenario timeout: '+$log) }
        if (-not $process.WaitForExit(120000) -or $process.ExitCode -ne 0) { throw ('Terrain scenario shutdown failed: '+$log) }
    }
    if ($Rendered) {
        foreach ($name in @('terrain-controls.png','terrain-overview.png')) {
            $path=Join-Path $directory $name
            if (-not (Test-Path $path) -or (Get-Item $path).Length -lt 1000) { throw ('Missing screenshot '+$path) }
        }
    }
    [ordered]@{passed=$true;map=$Map;rendered=[bool]$Rendered;restart=$true;run_id=$runId} | ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('TERRAIN_SCENARIO PASS '+$directory)
} finally {
    foreach ($process in $processes) { if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue } }
}
