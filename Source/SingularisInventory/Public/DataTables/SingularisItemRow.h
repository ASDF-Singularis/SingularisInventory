#pragma once

#include <CoreMinimal.h>

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USingularisItem> ItemClass = nullptr;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (MustImplement = "/Script/SingularisInventory.SingularisItemFormActorInterface")
	)
	TSubclassOf<AActor> FormActorClass = nullptr;
};
