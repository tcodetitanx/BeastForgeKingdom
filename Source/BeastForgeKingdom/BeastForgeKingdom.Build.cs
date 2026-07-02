// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BeastForgeKingdom : ModuleRules
{
	public BeastForgeKingdom(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });
	}
}

