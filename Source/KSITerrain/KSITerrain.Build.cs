using UnrealBuildTool;

public class KSITerrain : ModuleRules
{
    public KSITerrain(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "KSICore", "GeometryFramework", "GeometryCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "PhysicsCore", "Chaos", "ChaosCore" });
    }
}
