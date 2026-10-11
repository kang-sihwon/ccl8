using UnrealBuildTool;
public class KSICore : ModuleRules
{
    public KSICore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject" });
    }
}
