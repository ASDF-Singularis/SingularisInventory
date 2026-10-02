using UnrealBuildTool;

public class SingularisInventory : ModuleRules
{
	public SingularisInventory(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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

				"GameplayTags",
				"AssetRegistry",

				"EngineSettings",
				"DeveloperSettings",
			]
		);
	}
}