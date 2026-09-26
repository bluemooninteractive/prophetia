// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Vesperance : ModuleRules
{
	public Vesperance(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AssetRegistry", "SlateCore" });

		PublicIncludePaths.AddRange(new string[] {
			"Vesperance",
			"Vesperance/Variant_Strategy",
			"Vesperance/Variant_Strategy/UI",
			"Vesperance/Variant_TwinStick",
			"Vesperance/Variant_TwinStick/AI",
			"Vesperance/Variant_TwinStick/Gameplay",
			"Vesperance/Variant_TwinStick/UI",
			"Vesperance/Tactique"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
