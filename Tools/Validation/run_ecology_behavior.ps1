param(
    [ValidateSet('EnvironmentPlayground','EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [int]$Port = 20321,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/EcologyBehavior/{0}-{1}-{2}' -f $Map,$Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-Behavior([string]$Label, [string[]]$Options) {
    $log = Join-Path $directory ($Label+'.log')
    $arguments = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound',
        '-NullRHI','-NoLiveCoding','-CCLEcologyBehaviorSmoke',('-CCLEcologyBehaviorDir="'+$directory+'"'),
        ('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Behavior($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_ECOLOGY_BEHAVIOR FAIL|Fatal error:|Assertion failed:') { throw ('Ecology behavior failure: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Ecology behavior exited before verification: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Ecology behavior timeout: '+$Entry.Log)
}
function Wait-BehaviorExit($Entry) {
    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    while (-not $Entry.Process.HasExited -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 500 }
    if (-not $Entry.Process.HasExited -or $Entry.Process.ExitCode -ne 0) { throw ('Ecology behavior shutdown failed: '+$Entry.Log) }
}
try {
    $mapPath = '/Game/Maps/'+$Map
    $options = if ($Mode -eq 'Dedicated') { @($mapPath,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($mapPath+'?listen','-game',('-port='+$Port)) }
        else { @($mapPath,'-game') }
    $server = Start-Behavior 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-Behavior $server 'CCL_ECOLOGY_BEHAVIOR BASE_READY'
        $first = Start-Behavior 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-Behavior $first 'CCL_ECOLOGY_BEHAVIOR BASE_OBSERVED'
        'First client observed the initial Actors.' | Set-Content (Join-Path $directory 'first-ready.txt')
        Wait-Behavior $server 'CCL_ECOLOGY_BEHAVIOR RESTORE_READY'
        $late = Start-Behavior 'late' @(('127.0.0.1:'+$Port),'-game')
        Wait-Behavior $first 'CCL_ECOLOGY_BEHAVIOR PASS'
        Wait-Behavior $late 'CCL_ECOLOGY_BEHAVIOR PASS'
        'Both clients observed restored behavior poses.' | Set-Content (Join-Path $directory 'clients-ready.txt')
        Wait-BehaviorExit $first
        Wait-BehaviorExit $late
    }
    Wait-Behavior $server 'CCL_ECOLOGY_BEHAVIOR PASS'
    if ($Mode -eq 'Standalone') {
        Wait-BehaviorExit $server
        $reader = Start-Behavior 'reader' @($mapPath,'-game','-CCLEcologyBehaviorRead')
        Wait-Behavior $reader 'CCL_ECOLOGY_BEHAVIOR PASS'
        Wait-BehaviorExit $reader
    }
    [ordered]@{passed=$true;map=$Map;mode=$Mode;actor_mass=$true;save_restore=$true;food_history=$true;restart=($Mode -eq 'Standalone');clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone')} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('ECOLOGY_BEHAVIOR PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
