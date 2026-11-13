param([string]$Settlement='')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -Path "$PSScriptRoot/Assemble.cs" -ReferencedAssemblies @('System.Drawing.Common','System.Drawing.Primitives','System.Runtime','System.Collections','System.Runtime.InteropServices','System.Private.Windows.GdiPlus','System.Private.Windows.Core')
$plan=Get-Content "$PSScriptRoot/records/plan.json" -Raw|ConvertFrom-Json
foreach($s in $plan.settlements){
 if($s.id -eq 'M01' -or ($Settlement -and $s.id -ne $Settlement)){continue}
 $jobs=@($plan.jobs|Where-Object settlement -eq $s.id)
 if(@($jobs|Where-Object{!(Test-Path (Join-Path $PSScriptRoot $_.generated))}).Count){continue}
 $b=[double[]]@(($jobs|ForEach-Object{$_.worldPixelBounds[0]}|Measure-Object -Minimum).Minimum,($jobs|ForEach-Object{$_.worldPixelBounds[1]}|Measure-Object -Minimum).Minimum,($jobs|ForEach-Object{$_.worldPixelBounds[2]}|Measure-Object -Maximum).Maximum,($jobs|ForEach-Object{$_.worldPixelBounds[3]}|Measure-Object -Maximum).Maximum)
 [SettlementAssembly]::Build($PSScriptRoot,$s.id,[string[]]$jobs.id,[int[]]$jobs.column,[int[]]$jobs.row,[double[]]$s.contextBounds,$b)
 Write-Output "$($s.id): assembled $($jobs.Count) tiles"
}
