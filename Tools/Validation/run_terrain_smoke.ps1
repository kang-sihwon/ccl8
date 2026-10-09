param(
    [switch]$Rendered,
    [ValidateRange(30, 600)]
    [int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editorName = if ($Rendered) { 'UnrealEditor.exe' } else { 'UnrealEditor-Cmd.exe' }
$editor = Join-Path $paths.engineRoot ('Binaries/Win64/' + $editorName)
$project = Join-Path $repository 'CCL.uproject'
$directory = Join-Path $repository ('Saved/Tests/Terrain/{0}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'game.log'
$arguments = @(('"{0}"' -f $project), '/Game/Maps/EnvironmentScenario', '-game', '-unattended', '-nosplash', '-nosound',
    '-nop4', '-NoLiveCoding', '-CCLTerrainSmoke', '-ExecCmds="t.MaxFPS 60"', ('-abslog="{0}"' -f $log))
if ($Rendered) {
    $arguments += @('-windowed', '-ResX=1280', '-ResY=720', '-ForceRes', ('-CCLTerrainScreenshot="{0}"' -f (Join-Path $directory 'terrain.png')))
} else {
    $arguments += '-nullrhi'
}
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 500
    }
    if (-not $process.HasExited) { throw "Terrain smoke timeout: $log" }
    $content = Get-Content -LiteralPath $log -Raw
    if ($process.ExitCode -ne 0 -or $content -match 'CCL_TERRAIN_SMOKE FAIL' -or $content -notmatch 'CCL_TERRAIN_SMOKE PASS') {
        throw "Terrain smoke failed (exit $($process.ExitCode)): $log"
    }
    [ordered]@{passed=$true; exit_code=$process.ExitCode; log=$log; runtime='Editor game mode'; rendered=[bool]$Rendered; network_verified=$false} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output "TERRAIN_SMOKE PASS $directory"
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
}
