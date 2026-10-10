param(
    [ValidateSet('EnvironmentPlayground','EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/Vegetation/{0}-{1}' -f $Map,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
try {
    foreach ($phase in @('initial','restart')) {
        $log = Join-Path $directory ($phase+'.log')
        $options = @(('"'+$root+'/CCL.uproject"'),('/Game/Maps/'+$Map),'-game','-unattended','-nop4','-nosplash','-nosound',
            '-NullRHI','-NoLiveCoding','-CCLVegetationSmoke',('-CCLVegetationSmokeDir="'+$directory+'"'),
            ('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
        if ($phase -eq 'restart') { $options += '-CCLVegetationResume' }
        $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $options -WindowStyle Hidden -PassThru
        $processes.Add($process)
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 500 }
        if (-not $process.HasExited) { throw ('Vegetation process timeout: '+$log) }
        $content = Get-Content -LiteralPath $log -Raw
        if ($process.ExitCode -ne 0 -or $content -match 'CCL_VEGETATION FAIL' -or $content -notmatch 'CCL_VEGETATION PASS') {
            throw ('Vegetation assertion or shutdown failure: '+$log)
        }
    }
    $saved = [regex]::Match((Get-Content (Join-Path $directory 'initial.log') -Raw),'CCL_VEGETATION SAVED world=([0-9.]+) crc=([0-9]+)')
    $restored = [regex]::Match((Get-Content (Join-Path $directory 'restart.log') -Raw),'CCL_VEGETATION RESTART_RESTORED world=([0-9.]+) crc=([0-9]+)')
    if (-not $saved.Success -or -not $restored.Success -or $saved.Groups[1].Value -ne $restored.Groups[1].Value -or $saved.Groups[2].Value -ne $restored.Groups[2].Value) {
        throw 'New process vegetation state differs from the saved completed generation'
    }
    [ordered]@{passed=$true;map=$Map;mode='Standalone';restart=$true;vegetation_crc=$saved.Groups[2].Value;world_seconds=$saved.Groups[1].Value;run_id=$runId} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('VEGETATION_SMOKE PASS '+$directory)
} finally {
    foreach ($process in $processes) {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
    }
}
