param(
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [ValidateSet('EnvironmentScenario','EnvironmentPlayground')][string]$Map = 'EnvironmentScenario',
    [int]$Port = 20421,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/FoodPickup/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-FoodPickup([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding','-CCLFoodPickupSmoke',
        ('-CCLFoodPickupSmokeDir="'+$directory+'"'),('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    $args += '-NullRHI'
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-FoodPickup($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_FOOD_PICKUP FAIL') { throw ('FoodPickup assertion: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('FoodPickup process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('FoodPickup timeout: '+$Entry.Log)
}
try {
    $mapPath = '/Game/Maps/' + $Map
    $options = if ($Mode -eq 'Dedicated') { @($mapPath,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($mapPath+'?listen','-game',('-port='+$Port)) }
        else { @($mapPath,'-game') }
    $server = Start-FoodPickup 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-FoodPickup $server 'CCL_FOOD_PICKUP BASE_READY'
        $first = Start-FoodPickup 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-FoodPickup $first 'CCL_FOOD_PICKUP BASE_OBSERVED'
        Set-Content -LiteralPath (Join-Path $directory 'base-observed.txt') -Value 'observed'
        Wait-FoodPickup $server 'CCL_FOOD_PICKUP READY'
        $late = Start-FoodPickup 'late' @(('127.0.0.1:'+$Port),'-game','-CCLFoodPickupLate')
        Wait-FoodPickup $first 'CCL_FOOD_PICKUP PASS'
        Wait-FoodPickup $late 'CCL_FOOD_PICKUP PASS'
    }
    Wait-FoodPickup $server 'CCL_FOOD_PICKUP PASS'
    if ($Mode -eq 'Standalone') {
        if (-not $server.Process.WaitForExit(30000) -or $server.Process.ExitCode -ne 0) { throw 'FoodPickup shutdown failed' }
        $reader = Start-FoodPickup 'reader' @($mapPath,'-game','-CCLFoodPickupRead')
        Wait-FoodPickup $reader 'CCL_FOOD_PICKUP CONTINUATION_MATCH'
        Wait-FoodPickup $reader 'CCL_FOOD_PICKUP PASS'
        if (-not $reader.Process.WaitForExit(30000) -or $reader.Process.ExitCode -ne 0) { throw 'FoodPickup reader failed' }
    }
    [ordered]@{passed=$true;mode=$Mode;map=$Map;rendered=$false;clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone');run_id=$runId} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('FOOD_PICKUP PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
