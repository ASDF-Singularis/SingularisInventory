#pragma once

#include <CoreMinimal.h>

#include "InputActionValue.h"
#include "SingularisItemActionType.generated.h"

class AActor;
class AController;
class APawn;
class USingularisItem;
class USingularisInventoryComponent;

/**
 * 引力奇点物品动作上下文
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemActionContext
{
	GENERATED_BODY()

	/** 触发控制器（PlayerController）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AController* Controller = nullptr;

	/** 触发者（所控 Pawn）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	APawn* Instigator = nullptr;

	/** 承载者 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Avatar = nullptr;

	/** 被执行动作的物品实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USingularisItem* Item = nullptr;

	/** 触发方所属的库存调度器，供动作消费 / 操作口袋。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USingularisInventoryComponent* Inventory = nullptr;

	/** 触发输入值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInputActionValue InputValue{};
};
