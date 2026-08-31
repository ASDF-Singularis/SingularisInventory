#include "Configs/SingularisInventorySettings.h"

USingularisInventorySettings::USingularisInventorySettings()
{
}

#if WITH_EDITOR

FName USingularisInventorySettings::GetCategoryName() const
{
	return FName("Singularis");
}

FText USingularisInventorySettings::GetSectionText() const
{
	return NSLOCTEXT(
		"SingularisInventory",
		"SingularisInventorySettingsSectionText",
		"Singularis Inventory"
	);
}

FText USingularisInventorySettings::GetSectionDescription() const
{
	return NSLOCTEXT(
		"SingularisInventory",
		"SingularisInventorySettingsSectionDescription",
		"引力奇点库存插件设置"
	);
}

#endif
