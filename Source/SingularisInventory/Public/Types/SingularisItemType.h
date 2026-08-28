#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "SingularisItemType.generated.h"

class UInputAction;
class USingularisItemAction;

/**
 * 引力奇点物品动作输入
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemActionInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UInputAction> InputAction = nullptr;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (
			Categories = "Singularis.Inventory.ItemAction",
			ForceSelection = "true"
		)
	)
	FGameplayTag ActionTag{};
};

/**
 * 引力奇点物品动作条目
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemActionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ActionName{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ActionDescription{};

	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite)
	USingularisItemAction* Action = nullptr;
};

/**
 * 引力奇点物品动作管线：用于包装一组有序的动作
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemActionPipeline
{
	GENERATED_BODY()

	/** 有序动作数组，数组顺序即执行顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "ActionName"))
	TArray<FSingularisItemActionEntry> Actions{};

	/** 控制中断：首个动作失败时是否停止后续动作。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSuspend = true;
};
