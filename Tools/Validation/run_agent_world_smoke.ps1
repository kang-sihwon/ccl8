param([ValidateRange(30, 600)][int]$TimeoutSeconds = 180)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$directory = Join-Path $root ('Saved/Tests/AgentWorld/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'editor.log'
$options = @(
    ('"' + (Join-Path $root 'CCL.uproject') + '"'), '/Game/Maps/Campaign', '-game',
    '-CCLAgentSmoke', '-unattended', '-nop4', '-nosplash', '-NullRHI', '-NoLiveCoding',
    '-ExecCmds="t.MaxFPS 60"', ('-abslog="' + $log + '"')
)
$process = Start-Process -FilePath $editor -ArgumentList $options -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (!$process.HasExited) {
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Agent world timeout: ' + $log) }
        Start-Sleep -Milliseconds 500
    }
    $text = Get-Content -LiteralPath $log -Raw
    if ($process.ExitCode -ne 0 -or $text -notmatch 'CCL_AGENT_WORLD PASS' -or
        $text -match 'CCL_AGENT_WORLD FAIL|Assertion failed:|Fatal error:') {
        throw ('Agent world smoke failed: ' + $log)
    }
    Write-Output ('PASS agent world smoke: ' + $log)
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
