param([int]$Port = 20431, [string]$PackagedExecutable, [string]$ServerExecutable)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if ([bool]$PackagedExecutable -ne [bool]$ServerExecutable) { throw 'Provide both packaged Game and Server executables.' }
if ($PackagedExecutable) {
    $gameBinary = (Resolve-Path -LiteralPath $PackagedExecutable).Path
    $serverBinary = (Resolve-Path -LiteralPath $ServerExecutable).Path
} else {
    $paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
    $gameBinary = Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe'
    $serverBinary = $gameBinary
}
$directory = Join-Path $root ('Saved/Tests/InventoryNetwork/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$processes = [Collections.Generic.List[object]]::new()
function Start-Probe([string]$Role, [string[]]$ModeArgs) {
    $log = Join-Path $directory ($Role + '.log')
    [string[]]$prefix = if ($PackagedExecutable) { @() } else { @(('"' + $root + '/CCL.uproject"')) }
    $options = $prefix + $ModeArgs + @('-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding', '-NullRHI',
        '-CCLInventoryNetworkSmoke', ('-CCLInventoryNetDir="' + $directory + '"'), ('-CCLInventoryNetRole=' + $Role), ('-abslog="' + $log + '"'), '-ExecCmds="t.MaxFPS 60"')
    $binary = if ($Role -eq 'server') { $serverBinary } else { $gameBinary }
    $process = Start-Process -FilePath $binary -ArgumentList $options -WindowStyle Hidden -PassThru
    $entry = [pscustomobject]@{Process=$process; Log=$log}
    $processes.Add($entry)
    return $entry
}
function Wait-For([scriptblock]$Condition) {
    $deadline = [DateTime]::UtcNow.AddSeconds(180)
    while (!( & $Condition )) {
        foreach ($entry in $processes) {
            if (Test-Path $entry.Log) {
                $text = Get-Content $entry.Log -Raw
                if ($text -match 'CCL_INVENTORY_NET FAIL|LogUObjectGlobals: Error: CDO Constructor|Fatal error:|Assertion failed:|Ensure condition failed:') { throw ('Inventory network failure: ' + $entry.Log) }
                if ($entry.Process.HasExited -and $text -notmatch 'CCL_INVENTORY_NET PASS') { throw ('Inventory network exited early: ' + $entry.Log) }
            }
        }
        if ([DateTime]::UtcNow -gt $deadline) { throw ('Inventory network timeout: ' + $directory) }
        Start-Sleep -Milliseconds 500
    }
}
try {
    $server = Start-Probe 'server' @('/Game/Maps/CombatPlayground', '-server', ('-port=' + $Port))
    Wait-For { (Test-Path $server.Log) -and (Get-Content $server.Log -Raw) -match 'CCL_INVENTORY_NET READY' }
    $first = Start-Probe 'first' @(('127.0.0.1:' + $Port), '-game')
    Wait-For { Test-Path (Join-Path $directory 'first-updated') }
    $late = Start-Probe 'late' @(('127.0.0.1:' + $Port), '-game')
    Wait-For { (Test-Path (Join-Path $directory 'first-done')) -and (Test-Path (Join-Path $directory 'late-done')) }
    foreach ($entry in $processes) {
        if (!$entry.Process.WaitForExit(30000)) { throw ('Shutdown timeout: ' + $entry.Log) }
        $text = Get-Content $entry.Log -Raw
        if ($entry.Process.ExitCode -ne 0 -or $text -notmatch 'CCL_INVENTORY_NET PASS' -or $text -match 'CCL_INVENTORY_NET FAIL|LogUObjectGlobals: Error: CDO Constructor|Fatal error:|Assertion failed:|Ensure condition failed:') {
            throw ('Inventory network failed: ' + $entry.Log)
        }
    }
    [ordered]@{passed=$true;mode='Dedicated';packaged=[bool]$PackagedExecutable;clients=2;late_join=$true;owner_only=$true;state_update=$true} | ConvertTo-Json | Set-Content (Join-Path $directory 'result.json') -Encoding utf8
    Write-Output ('INVENTORY NETWORK PASS ' + $directory)
} finally {
    foreach ($entry in $processes) {
        if (!$entry.Process.HasExited) { Stop-Process -Id $entry.Process.Id -Force }
    }
}
