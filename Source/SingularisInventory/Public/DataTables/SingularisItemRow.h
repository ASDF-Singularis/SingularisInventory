#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "Types/SingularisItemType.h"
#include "SingularisItemRow.generated.h"

class USingularisItem;

USTRUCT(BlueprintType)
struct FSingularisItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Id = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Name{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon = nullptr;

	/**
	 * 物品动作映射：按动作标签匹配到动作管线。
	 * 键为动作标签（支持层级匹配），值为有序执行的动作管线。
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (
			DisplayName = "动作映射",
			Categories = "Singularis.Inventory.ItemAction",
			ForceSelection = "true"
		)
	)
	TMap<FGameplayTag, FSingularisItemActionPipeline> ItemActionMapping{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USingularisItem> ItemClass = nullptr;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (MustImplement = "/Script/SingularisInventory.SingularisItemFormActorInterface")
	)
	TSubclassOf<AActor> FormActorClass = nullptr;
};
