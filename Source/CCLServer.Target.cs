using UnrealBuildTool;

public class CCLServerTarget : TargetRules
{
	public CCLServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.Add("CCL");
	}
}
