using UnrealBuildTool;

public class KSIUI : ModuleRules
{
    public KSIUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayTags", "UMG", "CommonUI", "SlateCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "InputCore", "CommonInput", "EnhancedInput" });
    }
}
