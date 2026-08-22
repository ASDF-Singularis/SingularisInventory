#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>
#include <UObject/SoftObjectPath.h>

#include "Templates/SubclassOf.h"
#include "SingularisInventorySettings.generated.h"

UCLASS(Config = SingularisInventory, DefaultConfig)
class SINGULARISINVENTORY_API USingularisInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(
		Config,
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|参数",
		meta = (
			DisplayName = "物品注册表",
			RequiredAssetDataTags = "RowStructure=/Script/SingularisInventory.SingularisItemRow"
		)
	)
	TSoftObjectPtr<UDataTable> ItemTable = nullptr;

	USingularisInventorySettings();

#if WITH_EDITOR

	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

#endif
};
