param(
    [string]$PackagedExecutable,
    [switch]$Offscreen,
    [ValidateRange(1280, 3840)][int]$Width = 1280,
    [ValidateRange(720, 2160)][int]$Height = 720
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if ($PackagedExecutable) {
    $executable = (Resolve-Path -LiteralPath $PackagedExecutable).Path
    [string[]]$prefix = @()
} else {
    $paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
    $executable = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
    [string[]]$prefix = @(('"' + (Join-Path $root 'CCL.uproject') + '"'))
}
$directory = Join-Path $root ('Saved/Tests/UIVisual/{0}x{1}-{2}' -f $Width, $Height, (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'game.log'
$options = $prefix + @('/Game/Maps/Campaign', '-game', '-CCLUISmoke', '-unattended', '-nosplash', '-nosound', '-NoLiveCoding', '-windowed', '-ForceRes', "-ResX=$Width", "-ResY=$Height", ('-CCLUICapture="' + $directory + '"'), ('-abslog="' + $log + '"'), '-ExecCmds="t.MaxFPS 60,r.MotionBlurQuality 0"')
if ($Offscreen) { $options += '-RenderOffScreen' }
$process = Start-Process -FilePath $executable -ArgumentList $options -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(240)
    while (!$process.HasExited) {
        if ([DateTime]::UtcNow -gt $deadline) { throw ('UI test timeout: ' + $log) }
        Start-Sleep -Milliseconds 500
    }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'CCL_UI PASS' -Quiet) -or (Select-String -LiteralPath $log -Pattern 'CCL_UI FAIL|Fatal error:|Assertion failed:' -Quiet)) {
        Get-Content -LiteralPath $log -Tail 30
        throw ('UI test failed: ' + $log)
    }
    Write-Output ("PASS UI ${Width}x${Height}: " + $directory)
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
