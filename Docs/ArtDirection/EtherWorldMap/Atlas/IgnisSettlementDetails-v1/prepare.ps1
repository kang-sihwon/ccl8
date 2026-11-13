param([string]$Settlement='')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -Path "$PSScriptRoot/GridRaster.cs" -ReferencedAssemblies @('System.Drawing.Common','System.Drawing.Primitives','System.Runtime','System.Collections','System.Runtime.InteropServices','System.Private.Windows.GdiPlus','System.Private.Windows.Core')
$plan=Get-Content "$PSScriptRoot/records/plan.json" -Raw|ConvertFrom-Json
if(!(Test-Path "$PSScriptRoot/contexts/source-V01.png")){
 $svg=Get-Content "$PSScriptRoot/../../Images/veles-map-ui-unified-v2.svg" -Raw
 $encoded=[regex]::Match($svg,'data:image/png;base64,([^"'']+)').Groups[1].Value
 $stream=[IO.MemoryStream]::new([Convert]::FromBase64String($encoded))
 $image=[Drawing.Bitmap]::new($stream)
 $crop=$image.Clone([Drawing.Rectangle]::new(0,50,1672,841),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
 $crop.Save("$PSScriptRoot/contexts/source-V01.png",[Drawing.Imaging.ImageFormat]::Png)
 $crop.Dispose();$image.Dispose();$stream.Dispose()
}
foreach($s in $plan.settlements){if(!$s.sourceBounds -or ($Settlement -and $s.id -ne $Settlement)){continue};$target=Join-Path $PSScriptRoot $s.context;if(!(Test-Path $target)){[GridRaster]::Warp((Join-Path $PSScriptRoot $s.source),$target,[double[]]$s.sourceBounds,[double[]]$s.contextBounds,2048,1024)}}
foreach($j in $plan.jobs){if($Settlement -and $j.settlement -ne $Settlement){continue};$target=Join-Path $PSScriptRoot $j.guide;if(!(Test-Path $target)){$b=$j.worldPixelBounds;$m=$j.worldMargin;[GridRaster]::Warp((Join-Path $PSScriptRoot $j.source),$target,[double[]]$j.sourceBounds,[double[]]@(($b[0]-$m),($b[1]-$m),($b[2]+$m),($b[3]+$m)),2304,2304)}}
$items=@($plan.jobs|Where-Object{!$Settlement -or $_.settlement -eq $Settlement});for($i=0;$i -lt $items.Count;$i+=12){$batch=@($items|Select-Object -Skip $i -First 12);[GridRaster]::Contact([string[]]@($batch|ForEach-Object{Join-Path $PSScriptRoot $_.guide}),[string[]]@($batch|ForEach-Object{"$($_.settlement) $($_.id)"}),"$PSScriptRoot/previews/guides-$Settlement-$i.png",4,384)}
Write-Output "Prepared $($items.Count) guides."
