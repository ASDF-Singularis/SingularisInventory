using UnrealBuildTool;

public class SingularisInventoryGameplay : ModuleRules
{
	public SingularisInventoryGameplay(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			[
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",

				"SingularisInventory",

				"InputCore",
				"EnhancedInput",

				"GameplayTags"
			]
		);
	}
}