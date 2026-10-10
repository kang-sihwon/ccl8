param(
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [int]$Port = 19891,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/Fire/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-Fire([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding','-CCLFireSmoke',
        ('-CCLFireSmokeDir="'+$directory+'"'),('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $args += '-NullRHI'
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-Fire($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_FIRE_SMOKE FAIL') { throw ('Fire assertion: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('Fire process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('Fire timeout: '+$Entry.Log)
}
try {
    $map = '/Game/Maps/EnvironmentScenario'
    $options = if ($Mode -eq 'Dedicated') { @($map,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($map+'?listen','-game',('-port='+$Port)) }
        else { @($map,'-game') }
    $server = Start-Fire 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-Fire $server 'CCL_FIRE_SMOKE BASE_READY'
        $first = Start-Fire 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-Fire $server 'CCL_FIRE_SMOKE RESTORE_READY'
        $late = Start-Fire 'late' @(('127.0.0.1:'+$Port),'-game')
        Wait-Fire $first 'CCL_FIRE_SMOKE PASS'
        Wait-Fire $late 'CCL_FIRE_SMOKE PASS'
    }
    Wait-Fire $server 'CCL_FIRE_SMOKE PASS'
    if ((Get-Content $server.Log -Raw) -notmatch 'CCL_FIRE_SMOKE GEOMETRY_READY') { throw 'Missing actual geometry obstruction/reopening verification' }
    if ($Mode -ne 'Standalone') {
        $pattern = 'combustion burned=([-0-9.]+) smoke=([-0-9.]+) melt=([-0-9.]+) air=([-0-9.]+) exported=([-0-9.]+)'
        $serverMatch = [regex]::Match((Get-Content $server.Log -Raw), $pattern)
        if (-not $serverMatch.Success) { throw 'Missing committed server fire values' }
        foreach ($client in @($first,$late)) {
            $clientMatch = [regex]::Match((Get-Content $client.Log -Raw), $pattern)
            if (-not $clientMatch.Success) { throw ('Missing client fire values: '+$client.Log) }
            foreach ($field in 1..5) {
                if ($clientMatch.Groups[$field].Value -ne $serverMatch.Groups[$field].Value) {
                    throw ('Client fire differs from committed server combustion totals: '+$client.Log)
                }
            }
        }
    }
    if ($Mode -eq 'Standalone') {
        $shutdownDeadline = [DateTime]::UtcNow.AddSeconds(120)
        while (-not $server.Process.HasExited -and [DateTime]::UtcNow -lt $shutdownDeadline) { Start-Sleep -Milliseconds 500 }
        if (-not $server.Process.HasExited -or $server.Process.ExitCode -ne 0) { throw ('Fire shutdown failed: '+$server.Log) }
    }
    [ordered]@{passed=$true;geometry_occlusion=$true;mode=$Mode;clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone');run_id=$runId} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('FIRE_SMOKE PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
