// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class CCL : ModuleRules
{
	public CCL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// The runtime module references opt-in validation subsystems from Tests.
		bForceIncludeTestsFolder = true;
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks", "AIModule", "NavigationSystem", "StateTreeModule", "GameplayStateTreeModule", "MassEntity", "MassCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "Json", "JsonUtilities" });
		PrivateDependencyModuleNames.Add("GeometryCore");
		PrivateDependencyModuleNames.AddRange(new string[] { "PhysicsCore", "Chaos", "ChaosCore" });
		PublicDependencyModuleNames.Add("GeometryFramework");
		PublicDependencyModuleNames.Add("NetCore");
		PublicDependencyModuleNames.AddRange(new string[] { "UMG", "CommonUI", "CommonInput", "ModelViewViewModel", "FieldNotification" });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetRegistry", "StateTreeEditorModule", "PropertyBindingUtils", "PropertyEditor" });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "Json", "JsonUtilities" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
