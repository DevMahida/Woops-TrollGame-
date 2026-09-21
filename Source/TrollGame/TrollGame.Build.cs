// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TrollGame : ModuleRules
{
	public TrollGame(ReadOnlyTargetRules Target) : base(Target)
	{
		// Disable PCH creation to eliminate MSVC C3859 / 1455 PCH virtual memory allocation errors
		PCHUsage = PCHUsageMode.NoPCHs;
		bUseUnity = true;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TrollGame",
			"TrollGame/Traps/MathTrap",
			"TrollGame/Variant_Horror",
			"TrollGame/Variant_Horror/UI",
			"TrollGame/Variant_Shooter",
			"TrollGame/Variant_Shooter/AI",
			"TrollGame/Variant_Shooter/UI",
			"TrollGame/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
