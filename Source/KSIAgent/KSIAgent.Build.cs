using UnrealBuildTool;

public class KSIAgent : ModuleRules
{
    public KSIAgent(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayTags" });
    }
}
