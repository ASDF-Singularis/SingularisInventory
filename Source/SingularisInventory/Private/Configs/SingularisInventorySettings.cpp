#include "Configs/SingularisInventorySettings.h"

#include <UObject/ConstructorHelpers.h>

#include "SingularisInventory.h"

USingularisInventorySettings::USingularisInventorySettings()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> ItemTableFinder(
		TEXT(
			"/SingularisInventory/DataTables/DT_SingularisInventory_ItemTable.DT_SingularisInventory_ItemTable"
		)
	);

	if (ItemTableFinder.Succeeded())
	{
		ItemTable = ItemTableFinder.Object;
	}
	else
	{
		UE_LOG(LogSingularisInventory, Error, TEXT("默认物品数据表加载失败：%s"), TEXT("/SingularisInventory/DataTables/DT_SingularisInventory_ItemTable"));
	}
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
