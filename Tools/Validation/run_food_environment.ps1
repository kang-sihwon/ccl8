param(
    [ValidateSet('Standalone','Listen','Dedicated')][string]$Mode = 'Standalone',
    [switch]$Rendered,
    [ValidateSet('EnvironmentScenario','EnvironmentPlayground')][string]$Map = 'EnvironmentScenario',
    [int]$Port = 20411,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
if ($Rendered -and $Mode -ne 'Standalone') { throw 'Rendered inspection requires Standalone.' }
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/FoodEnvironment/{0}-{1}' -f $Mode,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$runId = [Guid]::NewGuid().ToString('N')
$processes = [Collections.Generic.List[object]]::new()
function Start-FoodEnvironment([string]$Name, [string[]]$Options) {
    $log = Join-Path $directory ($Name+'.log')
    $args = @(('"'+$root+'/CCL.uproject"')) + $Options + @('-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding','-CCLFoodEnvironmentSmoke',
        ('-CCLFoodEnvironmentSmokeDir="'+$directory+'"'),('-CCLExperimentRun='+$runId),'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$log+'"'))
    if ($Rendered -and $Mode -eq 'Standalone' -and $Name -eq 'server') { $args += @('-CCLFoodEnvironmentCapture','-windowed','-ForceRes','-ResX=1280','-ResY=720') }
    else { $args += '-NullRHI' }
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $args -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process;Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-FoodEnvironment($Entry, [string]$Pattern) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $Entry.Log) {
            $content = Get-Content -LiteralPath $Entry.Log -Raw
            if ($content -match 'CCL_FOOD_ENVIRONMENT FAIL') { throw ('FoodEnvironment assertion: '+$Entry.Log) }
            if ($content -match $Pattern) { return }
        }
        if ($Entry.Process.HasExited) { throw ('FoodEnvironment process exited early: '+$Entry.Log) }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    throw ('FoodEnvironment timeout: '+$Entry.Log)
}
try {
    $mapPath = '/Game/Maps/' + $Map
    $options = if ($Mode -eq 'Dedicated') { @($mapPath,'-server',('-port='+$Port)) }
        elseif ($Mode -eq 'Listen') { @($mapPath+'?listen','-game',('-port='+$Port)) }
        else { @($mapPath,'-game') }
    $server = Start-FoodEnvironment 'server' $options
    if ($Mode -ne 'Standalone') {
        Wait-FoodEnvironment $server 'CCL_FOOD_ENVIRONMENT BASE_READY'
        $first = Start-FoodEnvironment 'first' @(('127.0.0.1:'+$Port),'-game')
        Wait-FoodEnvironment $first 'CCL_FOOD_ENVIRONMENT BASE_OBSERVED'
        Set-Content -LiteralPath (Join-Path $directory 'base-observed.txt') -Value 'observed'
        Wait-FoodEnvironment $server 'CCL_FOOD_ENVIRONMENT RESTORE_READY'
        $late = Start-FoodEnvironment 'late' @(('127.0.0.1:'+$Port),'-game','-CCLFoodEnvironmentLate')
        Wait-FoodEnvironment $first 'CCL_FOOD_ENVIRONMENT PASS'
        Wait-FoodEnvironment $late 'CCL_FOOD_ENVIRONMENT PASS'
    }
    Wait-FoodEnvironment $server 'CCL_FOOD_ENVIRONMENT PASS'
    if ($Mode -eq 'Standalone') {
        $shutdownDeadline = [DateTime]::UtcNow.AddSeconds(120)
        while (-not $server.Process.HasExited -and [DateTime]::UtcNow -lt $shutdownDeadline) { Start-Sleep -Milliseconds 500 }
        if (-not $server.Process.HasExited -or $server.Process.ExitCode -ne 0) { throw ('FoodEnvironment shutdown failed: '+$server.Log) }
        $reader = Start-FoodEnvironment 'reader' @($mapPath,'-game','-CCLFoodEnvironmentRead')
        Wait-FoodEnvironment $reader 'CCL_FOOD_ENVIRONMENT CONTINUATION_MATCH'
        Wait-FoodEnvironment $reader 'CCL_FOOD_ENVIRONMENT PASS'
        if (-not $reader.Process.WaitForExit(30000) -or $reader.Process.ExitCode -ne 0) { throw 'FoodEnvironment reader shutdown failed' }
    }
    if ($Rendered) {
        foreach ($name in @('food-environment-controls.png','food-environment-states.png')) {
            $path = Join-Path $directory $name
            if (-not (Test-Path $path) -or (Get-Item $path).Length -lt 1000) { throw ('Missing screenshot '+$path) }
        }
    }
    [ordered]@{passed=$true;mode=$Mode;map=$Map;rendered=[bool]$Rendered;clients=$(if ($Mode -eq 'Standalone') {0} else {2});late_join=($Mode -ne 'Standalone');run_id=$runId} |
        ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding UTF8
    Write-Output ('FOOD_ENVIRONMENT PASS '+$directory)
} finally {
    foreach ($entry in $processes) {
        if (-not $entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
