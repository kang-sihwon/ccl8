using UnrealBuildTool;

public class KSISave : ModuleRules
{
    public KSISave(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core" });
    }
}
