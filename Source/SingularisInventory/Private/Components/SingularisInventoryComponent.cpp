#include "Components/SingularisInventoryComponent.h"

#include <Engine/DataTable.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include "Components/SingularisItemComponent.h"
#include "Components/SingularisPocketComponent.h"
#include "DataTables/SingularisItemRow.h"
#include "Objects/SingularisItem.h"

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

AActor* USingularisInventoryComponent::SpawnItemInWorld(const FName RowId, const FTransform Transform)
{
	// 1) 零信任校验：数据表必须有效
	if (!IsValid(ItemTable))
		return nullptr;

	const FSingularisMagicalElementRow* Row = ItemTable->FindRow<FSingularisMagicalElementRow>(
		RowId,
		TEXT("SpawnItemInWorld")
	);
	if (Row == nullptr)
		return nullptr;

	// 2) 行内物品类与形态 Actor 类必须配置
	if (!IsValid(Row->ItemClass) || !IsValid(Row->FormActorClass))
		return nullptr;

	// 3) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = GetWorld()->SpawnActor<AActor>(Row->FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
		return nullptr;

	// 4) 查找 ItemComponent；找到则构造并绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
	{
		// Outer 为瞬时包，物品按 UPROPERTY 引用计数存活，与形态 Actor 生命周期解耦
		USingularisItem* Item = NewObject<USingularisItem>(GetTransientPackage(), Row->ItemClass);
		if (IsValid(Item))
			ItemComponent->BindItem(Item);
	}

	return FormActor;
}

USingularisItem* USingularisInventoryComponent::CollectItem(
	USingularisItemComponent* ItemComponent,
	USingularisPocketComponent* TargetContainer
)
{
	// 1) 零信任校验：ItemComponent 必须有效
	if (!IsValid(ItemComponent))
		return nullptr;

	// 2) 取回物品实例；无物品则不销毁形态 Actor，直接返回
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
		return nullptr;

	// 3) 销毁形态 Actor
	AActor* FormActor = ItemComponent->GetOwner();
	if (IsValid(FormActor))
		FormActor->Destroy();

	// 4) 提供目标容器则尝试入容器；满或未提供容器时返回实例由调用方处置
	if (IsValid(TargetContainer))
		TargetContainer->AddItem(Item);

	return Item;
}
