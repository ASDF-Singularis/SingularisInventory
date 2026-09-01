#pragma once

#include <CoreMinimal.h>
#include <Engine/DataTable.h>

#include "SingularisItemFormRow.generated.h"

class AActor;

/**
 * 引力奇点物品形态行。
 *
 * 以物品标签（ItemTag）为行名的全局注册表：映射物品标签到形态 Actor 类，
 * 作为物品定义与形态 Actor 之间的唯一桥梁（单向、无相互引用）。
 */
USTRUCT(BlueprintType)
struct FSingularisItemFormRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "形态演员类"))
	TSubclassOf<AActor> FormActorClass = nullptr;
};
