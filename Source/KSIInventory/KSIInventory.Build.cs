using UnrealBuildTool;
public class KSIInventory : ModuleRules
{
    public KSIInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "NetCore", "KSICore" });
    }
}
