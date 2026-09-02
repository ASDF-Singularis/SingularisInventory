#pragma once

#include <CoreMinimal.h>

#include "InputActionValue.h"
#include "SingularisItemFragmentType.generated.h"

class AActor;
class AController;
class APawn;
class USingularisItem;

/**
 * 引力奇点物品片段上下文
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemFragmentContext
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

	/** 被执行片段所作用的物品实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USingularisItem* Item = nullptr;

	/** 触发输入值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInputActionValue InputValue{};
};
