#include "Actors/SingularisItemFormActor.h"

#include "SingularisInventory.h"
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

	UE_LOG(
		LogSingularisInventory,
		Verbose,
		TEXT("[%s] BeginPlay：形态 Actor 初始化完成，物品组件 %s"),
		*GetNameSafe(this),
		IsValid(ItemComponent) ? TEXT("已就绪") : TEXT("缺失")
	);
}

void ASingularisItemFormActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}
