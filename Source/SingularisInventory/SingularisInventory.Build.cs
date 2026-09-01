using UnrealBuildTool;

public class SingularisInventory : ModuleRules
{
	public SingularisInventory(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			[
				"GameplayTags"
			]
		);

		PrivateDependencyModuleNames.AddRange(
			[
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",
				"Projects",

				"RenderCore",
				"Renderer",
				"RHI",

				"UMG",
				"Slate",
				"SlateCore",

				"InputCore",
				"EnhancedInput",

				"EngineSettings",
				"DeveloperSettings"
			]
		);
	}
}