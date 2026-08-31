#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "Objects/SingularisItemDefinition.h"
#include "SingularisInventorySettings.generated.h"

UCLASS(Config = SingularisInventory, DefaultConfig)
class SINGULARISINVENTORY_API USingularisInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 物品定义注册表，系统按此扫描并解析物品定义。 */
	UPROPERTY(
		Config,
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|参数",
		meta = (DisplayName = "物品定义注册表")
	)
	TArray<TSoftObjectPtr<USingularisItemDefinition>> ItemDefinitions{};

	USingularisInventorySettings();

#if WITH_EDITOR

	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

#endif
};
