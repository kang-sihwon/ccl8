using UnrealBuildTool;

public class KSICelestial : ModuleRules
{
    public KSICelestial(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}
