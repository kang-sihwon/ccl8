using UnrealBuildTool;

public class KSICombat : ModuleRules
{
    public KSICombat(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags", "GameplayTasks" });
    }
}
