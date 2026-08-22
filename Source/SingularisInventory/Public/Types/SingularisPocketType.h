#pragma once

#include <CoreMinimal.h>

#include "Objects/SingularisItem.h"
#include "SingularisPocketType.generated.h"

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
