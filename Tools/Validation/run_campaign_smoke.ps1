param(
    [ValidateSet('Standalone', 'Dedicated', 'Listen')][string]$Mode = 'Standalone',
    [int]$Port = 18781,
    [switch]$DriverIsHost,
    [switch]$Impaired,
    [switch]$Rendered,
    [switch]$Progression,
    [switch]$Content,
    [ValidateRange(60, 1800)][int]$StartupTimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
if ($Content -and $Progression) { throw 'Run content and progression fixtures separately' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content -LiteralPath (Join-Path $repository '.local/agent-paths.json') -Raw | ConvertFrom-Json
$editor = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
if ($Rendered -and $Mode -ne 'Standalone') { throw 'Rendered snapshots require Standalone' }
$project = Join-Path $repository 'CCL.uproject'
$map = '/Game/Maps/Campaign'
$logDirectory = Join-Path $repository ('Saved/Tests/CampaignSmoke/{0}-{1}' -f $Mode, (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$started = [Collections.Generic.List[System.Diagnostics.Process]]::new()
$clients = [Collections.Generic.List[string]]::new()

function Start-Game([string]$Label, [string[]]$Options) {
    $log = Join-Path $logDirectory ($Label + '.log')
    $arguments = @(('"{0}"' -f $project)) + $Options + @(
        '-unattended', '-nosplash', '-nosound', '-NoLiveCoding', '-NoAsyncLoadingThread',
        ('-ExecCmds="t.MaxFPS 60' + $(if ($Rendered) { ',r.MotionBlurQuality 0' } else { '' }) + $(if ($Impaired) { ',NetEmulation.PktLag 50,NetEmulation.PktLoss 2' } else { '' }) + '"'), ('-abslog="{0}"' -f $log)
    )
    if ($Progression) { $arguments += '-CCLProgressionSmoke' }
    if ($Content) { $arguments += '-CCLContentSmoke' }
    if ($Rendered) { $arguments += @('-CCLCampaignCapture', '-windowed', '-ResX=1280', '-ResY=720') } else { $arguments += '-nullrhi' }
    $started.Add((Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru))
    return $log
}
function Wait-Marker([string]$Log, [string]$Marker, [int]$Seconds) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    do {
        foreach ($file in (Get-ChildItem -LiteralPath $logDirectory -Filter '*.log')) {
            if (Select-String -LiteralPath $file.FullName -Pattern 'CCL_CAMPAIGN FAIL|Fatal error:|Assertion failed:' -Quiet) {
                throw ('Campaign failure: ' + $file.FullName)
            }
        }
        if ((Test-Path -LiteralPath $Log) -and (Select-String -LiteralPath $Log -Pattern $Marker -Quiet)) { return }
        foreach ($process in $started) {
            if ($process.HasExited) { throw ('Process exited before test completed: ' + $process.Id) }
        }
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Timed out waiting for ' + $Marker + ': ' + $Log) }
        Start-Sleep -Milliseconds 500
    } while ($true)
}
try {
    if ($Mode -eq 'Standalone') {
        $server = Start-Game 'driver' @($map, '-game', '-CCLCampaignSmoke=driver')
        $clients.Add($server)
    } else {
        if ($Mode -eq 'Dedicated') {
            $server = Start-Game 'server' @($map, '-server', '-CCLCampaignSmoke=server', ('-port={0}' -f $Port))
        } else {
            $role = if ($DriverIsHost) { 'driver' } else { 'witness' }
            $server = Start-Game 'host' @(($map + '?listen'), '-game', ('-port={0}' -f $Port), ('-CCLCampaignSmoke={0}' -f $role))
            $clients.Add($server)
        }
        Wait-Marker $server ('IpNetDriver listening on port ' + $Port) $StartupTimeoutSeconds
        if ($Mode -eq 'Dedicated') {
            $clients.Add((Start-Game 'witness' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLCampaignSmoke=witness')))
        }
        $role = if ($Mode -eq 'Listen' -and $DriverIsHost) { 'witness' } else { 'driver' }
        $clients.Add((Start-Game $role @(('127.0.0.1:{0}' -f $Port), '-game', ('-CCLCampaignSmoke={0}' -f $role))))
    }
    Wait-Marker $server 'CCL_CAMPAIGN SERVER PASS' ($StartupTimeoutSeconds + 180)
    if ($Progression) { Wait-Marker $server 'CCL_PROGRESSION PERSISTENCE PASS' 15 }
    if ($Content) { Wait-Marker $server 'CCL_CONTENT REWARD PASS' 15 }
    foreach ($client in $clients) { Wait-Marker $client 'CCL_CAMPAIGN CLIENT PASS' 60 }
    if ($Mode -ne 'Standalone') {
        $late = Start-Game 'late' @(('127.0.0.1:{0}' -f $Port), '-game', '-CCLCampaignSmoke=late')
        Wait-Marker $late 'CCL_CAMPAIGN CLIENT PASS role=late' $StartupTimeoutSeconds
    }
    Write-Output ('PASS ' + $Mode + ' impaired=' + $Impaired + ' hostDriver=' + $DriverIsHost)
    Select-String -LiteralPath $server -Pattern 'CCL_CAMPAIGN' | ForEach-Object { $_.Line }
} finally {
    foreach ($process in $started) {
        if (!$process.HasExited) { $process.Kill() }
        $process.Dispose()
    }
    Write-Output ('Logs: ' + $logDirectory)
}
