$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/UIFoundation/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'game.log'
$options = @(
    ('"' + (Join-Path $root 'CCL.uproject') + '"'), '/Game/Maps/Campaign',
    '-game', '-CCLUIFoundationSmoke', '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-RenderOffScreen', '-windowed', '-ForceRes', '-ResX=1280', '-ResY=720',
    '-ExecCmds="t.MaxFPS 60"', ('-abslog="' + $log + '"')
)
$process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $options -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(240)
    while (!$process.HasExited) {
        if ([DateTime]::UtcNow -gt $deadline) { throw ('UI foundation timeout: ' + $log) }
        Start-Sleep -Milliseconds 500
    }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'CCL_UI_FOUNDATION PASS' -Quiet) -or
        (Select-String -LiteralPath $log -Pattern 'CCL_UI_FOUNDATION FAIL|LogUIActionRouter: Error:|Fatal error:|Assertion failed:' -Quiet)) {
        Get-Content -LiteralPath $log -Tail 35
        throw ('UI foundation failed: ' + $log)
    }
    Write-Output ('PASS UI foundation: ' + $directory)
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
