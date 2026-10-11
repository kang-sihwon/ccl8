param([ValidateSet('KSIUI', 'KSIMap', 'KSICore', 'KSIInventory', 'KSICombat', 'KSIWorldTime', 'KSICelestial', 'KSITerrain', 'KSIAgent', 'KSIEnvironment', 'KSISave')][string]$Module = 'KSIUI')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$paths = Get-Content (Join-Path $root '.local/agent-paths.json') -Raw | ConvertFrom-Json
$directory = Join-Path $root ('Saved/Tests/FrameworkIsolation/' + $Module + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$source = Join-Path $directory 'Source'
New-Item -ItemType Directory -Path $source -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root ('Source/' + $Module)) -Destination $source -Recurse
$extraModules = @()
switch ($Module) {
    'KSIInventory' { $extraModules = @('KSICore') }
    'KSIWorldTime' { $extraModules = @('KSICore') }
    'KSITerrain' { $extraModules = @('KSICore') }
    'KSIEnvironment' { $extraModules = @('KSICore', 'KSICelestial') }
}
foreach ($extra in $extraModules) { Copy-Item -LiteralPath (Join-Path $root ('Source/' + $extra)) -Destination $source -Recurse }
$worldModules = @('KSIWorldTime', 'KSICelestial', 'KSITerrain', 'KSIAgent', 'KSIEnvironment', 'KSISave')
$world = [int]($Module -in $worldModules)
$probe = Join-Path $source 'KSIProbe'
New-Item -ItemType Directory -Path $probe | Out-Null
Copy-Item -Path (Join-Path $PSScriptRoot 'FrameworkProbe/*.h'), (Join-Path $PSScriptRoot 'FrameworkProbe/*.cpp') -Destination $probe
if ($Module -eq 'KSICombat') { Copy-Item -Path (Join-Path $PSScriptRoot 'FrameworkProbe/Combat/*') -Destination $probe }
if ($world) { Copy-Item -Path (Join-Path $PSScriptRoot 'FrameworkProbe/World/*') -Destination $probe }
$projectName = $Module + 'Probe'
$descriptor = [ordered]@{
    FileVersion = 3
    EngineAssociation = ''
    DisableEnginePluginsByDefault = $true
    Modules = @(@{Name='KSIProbe'; Type='Runtime'; LoadingPhase='Default'}, @{Name=$Module; Type='Runtime'; LoadingPhase='Default'})
    Plugins = @()
}
$dependencies = '"Core", "CoreUObject", "Engine", "' + $Module + '"'
$ui = 0
$map = 0
$core = 0
$inventory = 0
$combat = 0
if ($Module -eq 'KSIUI') {
    $ui = 1
    $dependencies += ', "GameplayTags", "EnhancedInput", "InputCore", "CommonUI"'
    $descriptor.Plugins = @(@{Name='CommonUI'; Enabled=$true}, @{Name='EnhancedInput'; Enabled=$true})
} elseif ($Module -eq 'KSIMap') { $map = 1 }
elseif ($Module -eq 'KSICore') { $core = 1 }
elseif ($Module -eq 'KSICombat') {
    $combat = 1
    $dependencies += ', "GameplayAbilities", "GameplayTags", "GameplayTasks"'
    $descriptor.Plugins = @(@{Name='GameplayAbilities'; Enabled=$true})
}
elseif ($Module -eq 'KSIInventory') {
    $inventory = 1
}
foreach ($extra in $extraModules) {
    $dependencies += ', "' + $extra + '"'
    $descriptor.Modules += @{Name=$extra; Type='Runtime'; LoadingPhase='Default'}
}
$worldDefinitions = ($worldModules | ForEach-Object {
    '        PublicDefinitions.Add("WITH_' + $_.ToUpperInvariant() + '=' + [int]($Module -eq $_) + '");'
}) -join "`n"
$project = Join-Path $directory ($projectName + '.uproject')
$descriptor | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $project -Encoding utf8
@"
using UnrealBuildTool;
public class KSIProbe : ModuleRules
{
    public KSIProbe(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { $dependencies });
        PublicDefinitions.Add("WITH_KSI_UI=$ui");
        PublicDefinitions.Add("WITH_KSI_MAP=$map");
        PublicDefinitions.Add("WITH_KSI_CORE=$core");
        PublicDefinitions.Add("WITH_KSI_INVENTORY=$inventory");
        PublicDefinitions.Add("WITH_KSI_COMBAT=$combat");
        PublicDefinitions.Add("WITH_KSI_WORLD=$world");
$worldDefinitions
    }
}
"@ | Set-Content -LiteralPath (Join-Path $probe 'KSIProbe.Build.cs') -Encoding utf8
@"
using UnrealBuildTool;
public class ${projectName}EditorTarget : TargetRules
{
    public ${projectName}EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("KSIProbe");
    }
}
"@ | Set-Content -LiteralPath (Join-Path $source ($projectName + 'Editor.Target.cs')) -Encoding utf8
$config = Join-Path $directory 'Config'
New-Item -ItemType Directory -Path $config | Out-Null
$settings = "[/Script/EngineSettings.GameMapsSettings]`nGameDefaultMap=/Engine/Maps/Entry`n"
if ($ui) { $settings += "`n[/Script/Engine.Engine]`nGameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient`n" }
$settings | Set-Content -LiteralPath (Join-Path $config 'DefaultEngine.ini') -Encoding utf8
if ($ui) {
    $inputSettings = "[/Script/Engine.InputSettings]`nDefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput`nDefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent`n"
    "[/Script/CommonInput.CommonInputSettings]`nbEnableEnhancedInputSupport=True`n" | Set-Content -LiteralPath (Join-Path $config 'DefaultGame.ini') -Encoding utf8
    $inputSettings | Set-Content -LiteralPath (Join-Path $config 'DefaultInput.ini') -Encoding utf8
}
$hashes = Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object { $_.FullName -notmatch '[\\/]KSIProbe[\\/]' } | ForEach-Object {
    @{path=$_.FullName.Substring($source.Length + 1); sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
}
$hashes | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $directory 'module-sha256.json') -Encoding utf8
try {
    $buildLog = Join-Path $directory 'build.log'
    & (Join-Path $paths.engineRoot 'Build/BatchFiles/Build.bat') ($projectName + 'Editor') Win64 Development ('-Project=' + $project) -WaitMutex -NoHotReloadFromIDE *> $buildLog
    if ($LASTEXITCODE -ne 0) { Get-Content $buildLog -Tail 55; throw ('Isolation build failed: ' + $buildLog) }
    $log = Join-Path $directory 'game.log'
    $options = @(('"' + $project + '"'), '/Engine/Maps/Entry', '-game', '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding', '-RenderOffScreen', '-windowed', '-ForceRes', '-ResX=640', '-ResY=480', ('-abslog="' + $log + '"'))
    $process = Start-Process -FilePath (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $options -WindowStyle Hidden -PassThru
    try {
        if (!$process.WaitForExit(180000)) { throw ('Isolation timeout: ' + $log) }
        $text = Get-Content -LiteralPath $log -Raw
        if ($process.ExitCode -ne 0 -or $text -notmatch 'KSI_PROBE PASS' -or $text -match 'KSI_PROBE FAIL|Fatal error:|Assertion failed:|LogEnhancedInput: Error:|LogUIActionRouter: Error:|LogUObjectGlobals: Error: CDO Constructor') {
            Get-Content $log -Tail 55
            throw ('Isolation runtime failed: ' + $log)
        }
        Write-Output ('PASS ' + $Module + ' isolation: ' + $directory)
    } finally {
        if (!$process.HasExited) { Stop-Process -Id $process.Id -Force }
    }

} finally {
    # A source-engine build may regenerate shared module manifests. Let UBT restore
    # the caller project's metadata before running its editor again.
    $engineManifest = Get-Content (Join-Path $paths.engineRoot 'Binaries/Win64/UnrealEditor.modules') -Raw | ConvertFrom-Json
    $projectManifestPath = Join-Path $root 'Binaries/Win64/UnrealEditor.modules'
    if (Test-Path -LiteralPath $projectManifestPath) {
        $projectManifest = Get-Content -LiteralPath $projectManifestPath -Raw | ConvertFrom-Json
        if ($engineManifest.BuildId -ne $projectManifest.BuildId) {
            $restoreLog = Join-Path $directory 'restore-ccl.log'
            & (Join-Path $paths.engineRoot 'Build/BatchFiles/Build.bat') CCLEditor Win64 Development ('-Project=' + (Join-Path $root 'CCL.uproject')) -WaitMutex -NoHotReloadFromIDE *> $restoreLog
            if ($LASTEXITCODE -ne 0) { throw ('CCL metadata restore failed: ' + $restoreLog) }
        }
    }
}
