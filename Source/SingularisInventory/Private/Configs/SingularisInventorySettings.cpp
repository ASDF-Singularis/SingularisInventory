#include "Configs/SingularisInventorySettings.h"

#include "SingularisInventory.h"

USingularisInventorySettings::USingularisInventorySettings()
{
	static ConstructorHelpers::FClassFinder<USingularisItem> ItemFinder(
		TEXT(
			"/SingularisInventory/BP_SingularisItem.BP_SingularisItem_C"
		)
	);
	static ConstructorHelpers::FObjectFinder<UDataTable> ItemTableFinder(
		TEXT(
			"/SingularisInventory/DataTables/DT_SingularisInventory_ItemForm.DT_SingularisInventory_ItemForm"
		)
	);

	if (ItemFinder.Succeeded())
		ItemClass = ItemFinder.Class;
	else
	{
		UE_LOG(
			LogSingularisInventory,
			Error,
			TEXT("默认物品类加载失败：%s"),
			TEXT("/SingularisInventory/BP_SingularisItem.BP_SingularisItem_C")
		);
	}

	if (ItemTableFinder.Succeeded())
		ItemFormTable = ItemTableFinder.Object;
	else
	{
		UE_LOG(
			LogSingularisInventory,
			Error,
			TEXT("默认物品形态注册表加载失败：%s"),
			TEXT("/SingularisInventory/DataTables/DT_SingularisInventory_ItemForm")
		);
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
