#pragma once

#include <CoreMinimal.h>

#include "SingularisItemType.generated.h"

class USingularisItemFragment;

/**
 * 引力奇点物品片段条目
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemFragmentEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText FragmentName{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText FragmentDescription{};

	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite)
	USingularisItemFragment* Fragment = nullptr;
};

/**
 * 引力奇点物品片段管线：用于包装一组有序的片段
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemFragmentPipeline
{
	GENERATED_BODY()

	/** 有序片段数组，数组顺序即执行顺序。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "FragmentName"))
	TArray<FSingularisItemFragmentEntry> Fragments{};

	/** 控制中断：首个片段失败时是否停止后续片段。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSuspend = true;
};
