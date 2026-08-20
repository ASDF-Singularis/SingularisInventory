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

AActor* USingularisInventoryComponent::SpawnItemInWorld(USingularisItem* Item, FTransform Transform)
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
	{
		return nullptr;
	}

	// 2) 按物品类查表取形态 Actor 类
	const TSubclassOf<AActor> FormActorClass = FindFormActorClassForItem(Item);
	if (!IsValid(FormActorClass))
	{
		return nullptr;
	}

	// 3) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = GetWorld()->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
	{
		return nullptr;
	}

	// 4) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
	{
		ItemComponent->BindItem(Item);
	}

	return FormActor;
}

USingularisItem* USingularisInventoryComponent::CollectItem(
	AActor* FormActor,
	USingularisPocketComponent* TargetContainer
)
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
	{
		return nullptr;
	}

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
	{
		return nullptr;
	}

	// 3) 取回物品实例；无物品则不销毁形态 Actor
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
	{
		return nullptr;
	}

	// 4) 销毁形态 Actor
	FormActor->Destroy();

	// 5) 提供目标容器则尝试入容器；满或未提供容器时返回实例由调用方处置
	if (IsValid(TargetContainer))
	{
		TargetContainer->AddItem(Item);
	}

	return Item;
}

TSubclassOf<AActor> USingularisInventoryComponent::FindFormActorClassForItem(USingularisItem* Item)
{
	// 1) 零信任校验：数据表与物品均需有效
	if (!IsValid(ItemTable) || !IsValid(Item))
	{
		return nullptr;
	}

	// 2) 遍历数据表，按物品类精确匹配查其形态 Actor 类
	for (const auto& Pair : ItemTable->GetRowMap())
	{
		const FSingularisItemRow* Row = reinterpret_cast<const FSingularisItemRow*>(Pair.Value);
		if (IsValid(Row->ItemClass) && Row->ItemClass.Get() == Item->GetClass())
		{
			return Row->FormActorClass;
		}
	}

	return nullptr;
}
