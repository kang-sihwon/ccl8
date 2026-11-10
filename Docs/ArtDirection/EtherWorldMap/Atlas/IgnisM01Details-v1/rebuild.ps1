$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -Path "$PSScriptRoot/AssembleDetail.cs" -ReferencedAssemblies @('System.Drawing.Common','System.Drawing.Primitives','System.Runtime','System.Collections','System.Runtime.InteropServices','System.Private.Windows.GdiPlus','System.Private.Windows.Core')
[AssembleDetail]::Build($PSScriptRoot)
[AssembleDetail]::Comparison($PSScriptRoot)
[AssembleDetail]::Repair($PSScriptRoot)
[AssembleDetail]::Seams($PSScriptRoot)
python "$PSScriptRoot/verify.py"
if($LASTEXITCODE -ne 0){throw 'Assembly verification failed'}
