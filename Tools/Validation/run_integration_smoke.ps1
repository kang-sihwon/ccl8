param([ValidateRange(30, 600)][int]$TimeoutSeconds = 240, [int]$Width = 1280, [int]$Height = 720)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$directory = Join-Path $root ('Saved/Tests/Integration/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'editor.log'
$options = @(
    ('"' + (Join-Path $root 'CCL.uproject') + '"'), '/Game/Maps/Campaign', '-game',
    '-CCLIntegrationSmoke', '-unattended', '-nop4', '-nosplash', '-RenderOffscreen', '-ForceRes', '-windowed', ('-ResX=' + $Width), ('-ResY=' + $Height), '-NoLiveCoding',
    '-ExecCmds="t.MaxFPS 60"', ('-abslog="' + $log + '"')
)
$process = Start-Process -FilePath $editor -ArgumentList $options -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (!$process.HasExited) {
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Integration timeout: ' + $log) }
        Start-Sleep -Milliseconds 500
    }
    $text = Get-Content -LiteralPath $log -Raw
    if ($process.ExitCode -ne 0 -or $text -notmatch 'CCL_INTEGRATION PASS' -or
        $text -match 'CCL_INTEGRATION FAIL|Assertion failed:|Fatal error:') {
        throw ('Integration smoke failed: ' + $log)
    }
    Write-Output ('PASS integration smoke: ' + $log)
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
