#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "Interfaces/SingularisItemFormActorInterface.h"
#include "SingularisItemFormActor.generated.h"

class USingularisItemComponent;

UCLASS(Abstract, Blueprintable)
class SINGULARISINVENTORY_API ASingularisItemFormActor : public AActor, public ISingularisItemFormActorInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<USingularisItemComponent> ItemComponent = nullptr;

	ASingularisItemFormActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual USingularisItemComponent* GetItemComponent_Implementation() override;
};
