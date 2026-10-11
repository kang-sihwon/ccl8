using UnrealBuildTool;

public class KSIWorldTime : ModuleRules
{
    public KSIWorldTime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "KSICore" });
    }
}
