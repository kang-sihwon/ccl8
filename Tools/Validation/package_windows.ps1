param([switch]$SkipBuild)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths=Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$uat=Join-Path $paths.engineRoot 'Build/BatchFiles/RunUAT.bat'
$arguments=@('BuildCookRun',('-project='+ (Join-Path $root 'CCL.uproject')), '-noP4','-platform=Win64','-clientconfig=Development','-cook','-stage','-pak','-iostore','-archive',('-archivedirectory='+ (Join-Path $root 'Saved/Packages')), '-utf8output','-unattended','-NoCompileEditor','-ubtargs=-MaxParallelActions=4')
if (!$SkipBuild) { $arguments+='-build' }
& $uat @arguments
if ($LASTEXITCODE -ne 0) { throw ('Packaging failed with exit code '+$LASTEXITCODE) }
