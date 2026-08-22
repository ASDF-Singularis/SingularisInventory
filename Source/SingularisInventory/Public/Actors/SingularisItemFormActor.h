#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "SingularisItemFormActor.generated.h"

class USingularisItemComponent;

UCLASS(Abstract, Blueprintable)
class SINGULARISINVENTORY_API ASingularisItemFormActor : public AActor
{
	GENERATED_BODY()

public:
#pragma region Instantiation

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<USingularisItemComponent> ItemComponent = nullptr;

#pragma endregion

#pragma region Constructors

	ASingularisItemFormActor();

#pragma endregion

#pragma region Actor Interface

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#pragma endregion
};
