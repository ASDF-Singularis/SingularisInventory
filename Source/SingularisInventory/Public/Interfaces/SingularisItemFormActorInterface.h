#pragma once

#include <CoreMinimal.h>
#include <UObject/Interface.h>

#include "SingularisItemFormActorInterface.generated.h"

class USingularisItemComponent;

UINTERFACE(Blueprintable, BlueprintType)
class USingularisItemFormActorInterface : public UInterface
{
	GENERATED_BODY()
};

class SINGULARISINVENTORY_API ISingularisItemFormActorInterface
{
	GENERATED_BODY()

public:
#pragma region SPI

	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品演员形态接口|SPI",
		meta = (DisplayName = "获取物品组件")
	)
	USingularisItemComponent* GetItemComponent();

#pragma endregion
};
