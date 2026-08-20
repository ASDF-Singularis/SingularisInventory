#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisInventoryComponent.generated.h"

UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物库存组件")
)
class SINGULARISINVENTORY_API USingularisInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Constructors

	USingularisInventoryComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

#pragma endregion
};
