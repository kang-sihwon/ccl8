param([ValidateSet('EnvironmentScenario','EnvironmentPlayground')][string]$Map = 'EnvironmentScenario')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/LoadoutRestore/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$profile = Join-Path $directory 'players.world'
function Run-Phase([string]$Name, [switch]$Reader) {
    $log = Join-Path $directory ($Name+'.log')
    $options = @(('"'+$root+'/CCL.uproject"'),('/Game/Maps/'+$Map),'-game','-NullRHI','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
        '-CCLLoadoutRestoreSmoke',('-CCLExperimentRun='+[Guid]::NewGuid().ToString('N')),('-abslog="'+$log+'"'),('-CCLPlayerProfileFile="'+$profile+'"'))
    if ($Reader) { $options += '-CCLPlayerProfileRead' }
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $options -WindowStyle Hidden -PassThru
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(240)
        while (!$process.HasExited) {
            if ([DateTime]::UtcNow -gt $deadline) { throw ('Loadout restore timeout: '+$log) }
            Start-Sleep -Milliseconds 500
        }
        $content = Get-Content -LiteralPath $log -Raw
        if ($process.ExitCode -ne 0 -or $content -match 'CCL_LOADOUT_RESTORE FAIL|Fatal error:|Assertion failed:' -or
            $content -notmatch 'CCL_LOADOUT_RESTORE PASS') { throw ('Loadout restore failed: '+$log) }
    } finally {
        if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
        $process.Dispose()
    }
}
Run-Phase 'game'
Run-Phase 'reader' -Reader
[ordered]@{passed=$true;map=$Map;writer_exit=0;reader_exit=0;fresh_world_profiles=$true} | ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
Write-Output ('LOADOUT RESTORE PASS '+$directory)
