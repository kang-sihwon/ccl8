param([int]$Start=0,[int]$Count=12)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -Path "$PSScriptRoot/GridRaster.cs" -ReferencedAssemblies @('System.Drawing.Common','System.Drawing.Primitives','System.Runtime','System.Collections','System.Runtime.InteropServices','System.Private.Windows.GdiPlus','System.Private.Windows.Core')
$plan=Get-Content "$PSScriptRoot/records/plan.json" -Raw|ConvertFrom-Json
$jobs=@($plan.jobs | Select-Object -Skip $Start -First $Count | Where-Object {Test-Path (Join-Path $PSScriptRoot $_.generated)})
if(!$jobs.Count){throw 'No generated images in this range.'}
$paths=[string[]]@($jobs|ForEach-Object{Join-Path $PSScriptRoot $_.generated})
$labels=[string[]]@($jobs|ForEach-Object{"$($_.settlement) $($_.id)"})
$destination=Join-Path $PSScriptRoot "previews/generated-$Start.png"
[GridRaster]::Contact($paths,$labels,$destination,4,384)
Write-Output $destination
