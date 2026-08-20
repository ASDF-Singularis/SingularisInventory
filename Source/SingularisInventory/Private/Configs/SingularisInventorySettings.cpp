#include "Configs/SingularisInventorySettings.h"

USingularisInventorySettings::USingularisInventorySettings()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> ItemTableFinder(
		TEXT(
			"/SingularisInventory/DataTables/DT_SingularisInventory_ItemTable.DT_SingularisInventory_ItemTable"
		)
	);

	if (ItemTableFinder.Succeeded())
		ItemTable = ItemTableFinder.Object;
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
