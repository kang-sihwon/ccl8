param([string]$PackagedExecutable, [switch]$Rendered, [switch]$Network, [switch]$Failure)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if ($PackagedExecutable) { $editor=(Resolve-Path -LiteralPath $PackagedExecutable).Path } else {
    $paths=Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
    $editor=Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
}
$directory=Join-Path $root ('Saved/Tests/SessionSmoke/'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$started=[Collections.Generic.List[System.Diagnostics.Process]]::new()
$runningByLog=@{}
function Start-Session([string]$Role) {
    $log=Join-Path $directory ($Role+'.log')
    [string[]]$prefix=if ($PackagedExecutable) { @() } else { @(('"'+(Join-Path $root 'CCL.uproject')+'"')) }
    $options=$prefix+@('-game','-unattended','-nosplash','-nosound','-NoLiveCoding','-NoAsyncLoadingThread','-port=18791',('-CCLSessionSmoke='+$Role),('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60,r.MotionBlurQuality 0"')
    if ($Rendered) { $options+=@('-CCLSessionCapture','-windowed','-ResX=1280','-ResY=720') } else { $options+='-nullrhi' }
    $process=Start-Process -FilePath $editor -ArgumentList $options -WindowStyle Hidden -PassThru
    $started.Add($process)
    $runningByLog[$log]=$process
    return $log
}
function Wait-Exit([System.Diagnostics.Process]$Process) {
    if (!$Process.WaitForExit(30000)) { throw 'Game did not exit after Quit' }
    if ($Process.ExitCode -ne 0) { throw ('Game exited with code '+$Process.ExitCode) }
}
function Wait-Session([string]$Log,[string]$Marker) {
    $deadline=[DateTime]::UtcNow.AddSeconds(240)
    do {
        if (Test-Path $Log) {
            if (Select-String -Path $Log -Pattern 'CCL_SESSION_TEST FAIL|Fatal error:|Assertion failed:' -Quiet) { throw ('Session failure: '+$Log) }
            if (Select-String -Path $Log -Pattern $Marker -Quiet) { return }
        }
        if ($runningByLog[$Log].HasExited) { throw ('Process exited before marker: '+$Log) }
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Timeout: '+$Log) }
        Start-Sleep -Milliseconds 500
    } while ($true)
}
try {
    if ($Network) {
        $hostLog=Start-Session 'host'
        Wait-Session $hostLog 'CCL_SESSION_TEST HOST READY'
        $joinLog=Start-Session 'join'
        Wait-Session $joinLog 'CCL_SESSION_TEST PASS join'
        Wait-Session $hostLog 'CCL_SESSION_TEST PASS host'
        Wait-Exit $runningByLog[$joinLog]
    } elseif ($Failure) {
        $log=Start-Session 'badjoin'
        Wait-Session $log 'CCL_SESSION_TEST PASS badjoin'
        Wait-Exit $runningByLog[$log]
    } else {
        $log=Start-Session 'write'
        Wait-Session $log 'CCL_SESSION_TEST PASS write'
        Wait-Exit $started[0]
        $log=Start-Session 'read'
        Wait-Session $log 'CCL_SESSION_TEST PASS read'
        Wait-Exit $runningByLog[$log]
    }
    Write-Output ('PASS Session network='+$Network+' failure='+$Failure+' rendered='+$Rendered)
} finally {
    foreach ($process in $started) {
        if (!$process.HasExited) { $process.Kill() }
        $process.Dispose()
    }
    Write-Output ('Logs: '+$directory)
}
