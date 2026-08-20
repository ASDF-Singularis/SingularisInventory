#include "Actors/SingularisItemFormActor.h"

#include "Components/SingularisItemComponent.h"

ASingularisItemFormActor::ASingularisItemFormActor()
{
	bReplicates = true;

	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	ItemComponent = CreateDefaultSubobject<USingularisItemComponent>(TEXT("ItemComponent"));
}

void ASingularisItemFormActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASingularisItemFormActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}
