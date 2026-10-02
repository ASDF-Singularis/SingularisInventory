#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "SingularisInventorySettings.generated.h"

class UDataTable;
class USingularisItem;

/**
 * 引力奇点库存插件设置。
 *
 * 插件级开发者设置（Project Settings → Singularis → Singularis Inventory）：
 * 指定物品实例类与物品形态注册表。仅物品实例类参与运行时；注册表当前无消费方。
 */
UCLASS(Config = SingularisInventory, DefaultConfig)
class SINGULARISINVENTORY_API USingularisInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 物品实例类：物化物品时使用的实例类；未配置回退 USingularisItem 基类。 */
	UPROPERTY(
		Config,
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "引力奇点物库存设置",
		meta = (DisplayName = "物品实例类")
	)
	TSubclassOf<USingularisItem> ItemClass = nullptr;

	/** 物品形态注册表：以物品标签为行名，映射到形态 Actor 类。 */
	UPROPERTY(
		Config,
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "引力奇点物库存设置",
		meta = (
			DisplayName = "物品形态注册表",
			RequiredAssetDataTags = "RowStructure=/Script/SingularisInventory.SingularisItemFormRow"
		)
	)
	TSoftObjectPtr<UDataTable> ItemFormTable = nullptr;

	USingularisInventorySettings();

#if WITH_EDITOR

	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

#endif
};
