using UnrealBuildTool;

public class SingularisInventory : ModuleRules
{
	public SingularisInventory(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			[
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",

				"InputCore",
				"EnhancedInput",

				"GameplayTags"
			]
		);
	}
}