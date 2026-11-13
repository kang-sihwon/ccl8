$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -Path "$PSScriptRoot/GridRaster.cs" -ReferencedAssemblies @('System.Drawing.Common','System.Drawing.Primitives','System.Runtime','System.Collections','System.Runtime.InteropServices','System.Private.Windows.GdiPlus','System.Private.Windows.Core')
$plan=Get-Content "$PSScriptRoot/records/plan.json" -Raw|ConvertFrom-Json
$paths=[string[]]@($plan.settlements|ForEach-Object{Join-Path $PSScriptRoot "previews/$($_.id)-ui.png"})
$labels=[string[]]@($plan.settlements|ForEach-Object{"$($_.id) $($_.name)"})
[GridRaster]::Gallery($paths,$labels,"$PSScriptRoot/previews/settlements-36.png",4,418,235)
Write-Output '36 settlement viewer captures assembled without changing aspect ratio.'
