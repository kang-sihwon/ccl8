param(
    [string]$Filter = 'CCL.Equipment.TagContracts',
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$directory = Join-Path $root ('Saved/Tests/Automation/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'editor.log'
$report = Join-Path $directory 'report'
$options = @(
    ('"' + (Join-Path $root 'CCL.uproject') + '"'),
    '-unattended', '-nop4', '-nosplash', '-NullRHI', '-NoLiveCoding',
    ('-ExecCmds="Automation RunTests ' + $Filter + '"'),
    '-TestExit="Automation Test Queue Empty"',
    ('-ReportExportPath="' + $report + '"'), ('-abslog="' + $log + '"')
)
$process = Start-Process -FilePath $editor -ArgumentList $options -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (!$process.HasExited) {
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Automation timeout: ' + $log) }
        Start-Sleep -Milliseconds 500
    }
    $result = Get-Content (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
    if ($process.ExitCode -ne 0 -or $result.failed -ne 0 -or $result.notRun -ne 0 -or $result.inProcess -ne 0 -or
        ($result.succeeded + $result.succeededWithWarnings) -lt 1) {
        throw ('Automation failed or incomplete: ' + $report)
    }
    Write-Output ('PASS automation ' + $Filter + ': ' + $report)
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
