#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "SingularisInventorySettings.generated.h"

class UDataTable;

UCLASS(Config = SingularisInventory, DefaultConfig)
class SINGULARISINVENTORY_API USingularisInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 物品形态注册表：以物品标签为行名，映射到形态 Actor 类。 */
	UPROPERTY(
		Config,
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|参数",
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
