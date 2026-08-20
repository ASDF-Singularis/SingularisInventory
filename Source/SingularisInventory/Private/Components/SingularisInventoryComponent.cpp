#include "Components/SingularisInventoryComponent.h"

#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include "Components/SingularisItemComponent.h"
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "Subsystems/SingularisInventoryItemSubsystem.h"

USingularisInventoryComponent::USingularisInventoryComponent()
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

AActor* USingularisInventoryComponent::SpawnItemInWorld(USingularisItem* Item, FTransform Transform)
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
		return nullptr;

	// 2) 经全局查询子系统取物品形态 Actor 类
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!IsValid(GameInstance))
		return nullptr;
	const USingularisInventoryItemSubsystem* ItemSubsystem = GameInstance->GetSubsystem<
		USingularisInventoryItemSubsystem>();
	if (!IsValid(ItemSubsystem))
		return nullptr;
	const TSubclassOf<AActor> FormActorClass = ItemSubsystem->GetFormActorClass(Item);
	if (!IsValid(FormActorClass))
		return nullptr;

	// 3) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = GetWorld()->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
		return nullptr;

	// 4) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
		ItemComponent->BindItem(Item);

	return FormActor;
}

USingularisItem* USingularisInventoryComponent::CollectItem(
	AActor* FormActor,
	USingularisPocketComponent* TargetContainer
)
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
		return nullptr;

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
		return nullptr;

	// 3) 取回物品实例；无物品则不销毁形态 Actor
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
		return nullptr;

	// 4) 销毁形态 Actor
	FormActor->Destroy();

	// 5) 提供目标容器则尝试入容器；满或未提供容器时返回实例由调用方处置
	if (IsValid(TargetContainer))
		TargetContainer->AddItem(Item);

	return Item;
}
