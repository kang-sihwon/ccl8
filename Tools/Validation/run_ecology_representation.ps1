param(
    [ValidateSet('EnvironmentPlayground','EnvironmentScenario')][string]$Map = 'EnvironmentScenario',
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [int]$Port = 19921,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/EcologyRepresentation/{0}-{1}-{2}' -f $Map,$Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-Representation([string]$Label, [string[]]$Options) {
    $log = Join-Path $directory ($Label+'.log')
    $arguments = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound',
        '-NullRHI','-NoLiveCoding','-CCLEcologyRepresentationSmoke',('-CCLEcologyRepresentationDir="'+$directory+'"'),
        ('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Representation($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_ECOLOGY_REPRESENTATION FAIL|Fatal error:|Assertion failed:') { throw ('Ecology representation failure: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Ecology representation exited before verification: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Ecology representation timeout: '+$Entry.Log)
}
function Wait-RepresentationExit($Entry) {
    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    while (-not $Entry.Process.HasExited -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 500 }
    if (-not $Entry.Process.HasExited -or $Entry.Process.ExitCode -ne 0) { throw ('Ecology representation shutdown failed: '+$Entry.Log) }
}
try {
    $mapPath = '/Game/Maps/'+$Map
    $options = if ($Mode -eq 'Dedicated') { @($mapPath,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($mapPath+'?listen','-game',('-port='+$Port)) }
        else { @($mapPath,'-game') }
    $server = Start-Representation 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-Representation $server 'CCL_ECOLOGY_REPRESENTATION BASE_READY'
        $first = Start-Representation 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-Representation $first 'CCL_ECOLOGY_REPRESENTATION OBSERVED_READY'
        $late = Start-Representation 'late' @(('127.0.0.1:'+$Port),'-game')
        Wait-Representation $late 'CCL_ECOLOGY_REPRESENTATION OBSERVED_READY'
        'Both actual clients observed the four server Actors.' | Set-Content (Join-Path $directory 'clients-ready.txt')
        Wait-Representation $first 'CCL_ECOLOGY_REPRESENTATION PASS'
        Wait-Representation $late 'CCL_ECOLOGY_REPRESENTATION PASS'
        Wait-RepresentationExit $first
        Wait-RepresentationExit $late
    }
    Wait-Representation $server 'CCL_ECOLOGY_REPRESENTATION PASS'
    if ($Mode -eq 'Standalone') { Wait-RepresentationExit $server }
    [ordered]@{passed=$true;map=$Map;mode=$Mode;actor_mass=$true;save_restore=$true;natural_death=$true;clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone')} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('ECOLOGY_REPRESENTATION PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
