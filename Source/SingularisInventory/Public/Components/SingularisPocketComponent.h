#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisPocketComponent.generated.h"

UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点口袋组件")
)
class SINGULARISINVENTORY_API USingularisPocketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Constructors

	USingularisPocketComponent();

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
