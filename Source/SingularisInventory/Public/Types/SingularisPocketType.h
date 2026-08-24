#pragma once

#include <CoreMinimal.h>

#include "Objects/SingularisItem.h"
#include "SingularisPocketType.generated.h"

/**
 * 引力奇点口袋占用状态
 */
UENUM(BlueprintType)
enum class ESingularisPocketOccupancy : uint8
{
	Empty UMETA(DisplayName = "空"),
	Partial UMETA(DisplayName = "部分占用"),
	Full UMETA(DisplayName = "已满"),
};

/**
 * 引力奇点口袋插槽
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisPocketSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USingularisItem* Item = nullptr;

	bool IsEmpty() const { return !IsValid(Item); }
};
