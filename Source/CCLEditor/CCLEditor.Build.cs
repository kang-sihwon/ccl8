using UnrealBuildTool;

public class CCLEditor : ModuleRules
{
    public CCLEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "CCL", "KSICombat", "KSIAgent", "UnrealEd", "Slate", "SlateCore", "PropertyEditor", "AssetRegistry", "StateTreeEditorModule", "PropertyBindingUtils", "Niagara", "NiagaraShader", "InputCore", "NavigationSystem", "StateTreeModule", "GameplayStateTreeModule", "GameplayTags" });
        PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Plugins/FX/Niagara/Source/Niagara/Internal"));
        PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Plugins/FX/Niagara/Source/NiagaraShader/Internal"));
    }
}
