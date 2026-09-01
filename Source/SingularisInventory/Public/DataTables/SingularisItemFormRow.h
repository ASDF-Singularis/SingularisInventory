#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Engine/DataTable.h>

#include "SingularisItemFormRow.generated.h"

class AActor;

/**
 * 引力奇点物品形态行。
 *
 * 以独立 ItemTag 字段为桥接键的全局注册表：映射物品标签到形态 Actor 类，
 * 作为物品定义与形态 Actor 之间的唯一桥梁（单向、无相互引用）。
 * 行名仅作资产行标识，与物品标签解耦。
 */
USTRUCT(BlueprintType)
struct FSingularisItemFormRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 物品标签：桥接键，限定在物品标签分类下。 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (
			DisplayName = "物品标签",
			Categories = "Singularis.Inventory.Item",
			ForceSelection = "true"
		)
	)
	FGameplayTag ItemTag{};

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (MustImplement = "/Script/SingularisInventory.SingularisItemFormActorInterface")
	)
	TSubclassOf<AActor> FormActorClass = nullptr;
};
