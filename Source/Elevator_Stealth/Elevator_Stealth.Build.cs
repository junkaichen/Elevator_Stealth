// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Elevator_Stealth : ModuleRules
{
	public Elevator_Stealth(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput" });
	}
}
