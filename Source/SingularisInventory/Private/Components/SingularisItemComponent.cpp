#include "Components/SingularisItemComponent.h"

#include <Net/UnrealNetwork.h>

#include "Objects/SingularisItem.h"

USingularisItemComponent::USingularisItemComponent()
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisItemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USingularisItemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1) 防御性清理：若调用方未通过 TakeItem 取回物品实例，解除复制注册避免悬挂引用
	UnregisterItemSubObject();
	Item = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USingularisItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USingularisItemComponent, Item);
}

USingularisItem* USingularisItemComponent::GetItem() const
{
	return Item.Get();
}

bool USingularisItemComponent::HasItem() const
{
	return Item != nullptr;
}

void USingularisItemComponent::BindItem(USingularisItem* InItem)
{
	// 1) 零信任校验：空入参直接忽略
	if (InItem == nullptr)
	{
		return;
	}

	// 2) 幂等：已持有同一实例则无副作用
	if (Item == InItem)
	{
		return;
	}

	// 3) 若已持有其他实例，先解除旧引用并广播取出，保证单一持有
	if (Item != nullptr)
	{
		UnregisterItemSubObject();
		OnItemReleasedEvent.Broadcast(Item.Get());
		Item = nullptr;
	}

	// 4) 建立新的持有关系，注册复制子对象并广播移入
	Item = InItem;
	RegisterItemSubObject();
	OnItemBoundEvent.Broadcast(InItem);
}

USingularisItem* USingularisItemComponent::TakeItem()
{
	// 1) 空状态安全返回
	if (Item == nullptr)
	{
		return nullptr;
	}

	// 2) 解除复制注册，广播取出并清空持有，将引用权交还调用方
	USingularisItem* const OutItem = Item.Get();
	UnregisterItemSubObject();
	OnItemReleasedEvent.Broadcast(OutItem);
	Item = nullptr;

	return OutItem;
}

void USingularisItemComponent::RegisterItemSubObject()
{
	if (Item == nullptr)
	{
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		AddReplicatedSubObject(Item.Get());
	}
}

void USingularisItemComponent::UnregisterItemSubObject()
{
	if (Item == nullptr)
	{
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		RemoveReplicatedSubObject(Item.Get());
	}
}
