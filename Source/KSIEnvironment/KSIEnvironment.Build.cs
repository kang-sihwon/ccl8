using UnrealBuildTool;

public class KSIEnvironment : ModuleRules
{
    public KSIEnvironment(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "KSICore", "KSICelestial" });
    }
}
