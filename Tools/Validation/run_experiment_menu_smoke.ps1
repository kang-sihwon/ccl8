param(
    [ValidateSet('EnvironmentPlayground', 'EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [switch]$PIE,
    [switch]$Lab,
    [ValidateRange(1280, 3840)][int]$Width = 1280,
    [ValidateRange(720, 2160)][int]$Height = 720,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
$mode = if ($PIE) { 'PIE' } else { 'Standalone' }
$directory = Join-Path $root ('Saved/Tests/ExperimentMenu/{0}-{1}-{2}' -f $Map, $mode, (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$log = Join-Path $directory 'editor.log'
$options = @(('"{0}"' -f (Join-Path $root 'CCL.uproject')), ('/Game/Maps/' + $Map),
    '-unattended', '-nosplash', '-nosound', '-nop4', '-NoLiveCoding', '-windowed', '-ForceRes', ('-ResX=' + $Width), ('-ResY=' + $Height),
    '-CCLExperimentSmoke', '-CCLExperimentMenuSmoke', '-CCLExperimentCapture',
    ('-CCLExperimentRun=' + [Guid]::NewGuid().ToString('N')), ('-CCLExperimentCaptureDir="{0}"' -f $directory), ('-abslog="{0}"' -f $log))
if ($Lab) { $options += '-CCLExperimentLabSmoke' }
if ($PIE) {
    $options += '-RenderOffScreen'
    $options += '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.LevelEditorPlaySettings]:PlayNumberOfClients=1,PlayNetMode=PIE_Standalone,GameGetsMouseControl=True'
    $script = (Join-Path $root 'Tools/Validation/start_ui_pie.py').Replace('\', '/')
    $options += ('-ExecCmds="py ' + $script + ',t.MaxFPS 60,r.MotionBlurQuality 0"')
} else {
    $options += @('-game', '-ExecCmds="t.MaxFPS 60,r.MotionBlurQuality 0"')
}
$process = Start-Process -FilePath $editor -ArgumentList $options -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (!$process.HasExited) {
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Menu input test timeout: ' + $log) }
        Start-Sleep -Milliseconds 500
    }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'CCL_EXPERIMENT_SMOKE PASS Role=MenuInput' -Quiet) -or
        (Select-String -LiteralPath $log -Pattern 'CCL_EXPERIMENT_SMOKE FAIL|Fatal error:|Assertion failed:' -Quiet)) {
        Get-Content -LiteralPath $log -Tail 35
        throw ('Menu input test failed: ' + $log)
    }
    $captures = @('menu-initial.png', 'menu-closed.png', 'menu-reopened.png')
    if ($Lab) { $captures += @('celestial-debug.png', 'celestial-rotated.png', 'terrain-cursor.png', 'terrain-edited.png', 'snapshot-restored.png') }
    foreach ($name in $captures) {
        $capture = Join-Path $directory $name
        if (!(Test-Path -LiteralPath $capture) -or (Get-Item -LiteralPath $capture).Length -lt 1000) { throw ('Missing capture: ' + $capture) }
    }
    [ordered]@{passed=$true;map=$Map;mode=$mode;lab=[bool]$Lab;width=$Width;height=$Height;log=$log} | ConvertTo-Json |
        Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ("EXPERIMENT MENU PASS $Map $mode $directory")
} finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
}
