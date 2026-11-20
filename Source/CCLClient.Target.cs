using UnrealBuildTool;

public class CCLClientTarget : TargetRules
{
	public CCLClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.Add("CCL");
	}
}
