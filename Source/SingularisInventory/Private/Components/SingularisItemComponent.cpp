#include "Components/SingularisItemComponent.h"

#include <Net/UnrealNetwork.h>

#include "SingularisInventory.h"
#include "Objects/SingularisItem.h"
#include "Objects/SingularisItemDefinition.h"
#include "Subsystems/SingularisInventorySubsystem.h"

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

	// 1) 设计期按 ItemTag 映射生成：仅权威端、编辑器加载的形态 Actor、尚未持有物品时执行
	//    SpawnItemInWorld 路径由调用方显式 BindItem，本分支不应触发
	if (GetOwner()->HasAuthority()
		&& GetOwner()->HasAllFlags(RF_WasLoaded)
		&& !HasItem())
	{
		// 2) 物品标签必须有效，否则无法映射到物品定义
		if (!ItemTag.IsValid())
		{
			UE_LOG(
				LogSingularisInventory,
				Warning,
				TEXT("[%s] BeginPlay：形态 Actor %s 未配置物品标签，无法生成"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(GetOwner()->GetClass())
			);
			return;
		}

		// 3) 经全局查询子系统按物品标签映射物品定义，单一数据源为物品定义资产
		const UGameInstance* const GameInstance = GetWorld()->GetGameInstance();
		if (!IsValid(GameInstance))
		{
			UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] BeginPlay：GameInstance 无效"), *GetNameSafe(GetOwner()));
			return;
		}
		const USingularisInventorySubsystem* const ItemSubsystem =
			GameInstance->GetSubsystem<USingularisInventorySubsystem>();
		if (!IsValid(ItemSubsystem))
		{
			UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] BeginPlay：物品查询子系统无效"), *GetNameSafe(GetOwner()));
			return;
		}
		USingularisItemDefinition* const Definition = ItemSubsystem->FindDefinitionByItemTag(ItemTag);
		if (!IsValid(Definition))
		{
			UE_LOG(
				LogSingularisInventory,
				Warning,
				TEXT("[%s] BeginPlay：物品标签 %s 未映射到物品定义，无法生成"),
				*GetNameSafe(GetOwner()),
				*ItemTag.ToString()
			);
			return;
		}

		// 4) 按映射得到的定义物化独立运行时实例并绑定
		USingularisItem* const Materialized = USingularisItem::MaterializeFromDefinition(GetWorld(), Definition);
		if (IsValid(Materialized))
			BindItem(Materialized);
	}
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
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] BindItem：物品实例为空"), *GetNameSafe(GetOwner()));
		return;
	}

	// 2) 幂等：已持有同一实例则无副作用
	if (Item == InItem)
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] BindItem：物品 %s 已持有，忽略"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(InItem)
		);
		return;
	}

	// 3) 若已持有其他实例，先解除旧引用并广播取出，保证单一持有
	if (Item != nullptr)
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] BindItem：替换旧物品 %s → %s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item.Get()),
			*GetNameSafe(InItem)
		);
		UnregisterItemSubObject();
		OnItemReleasedEvent.Broadcast(Item.Get());
		Item = nullptr;
	}

	// 4) 建立新的持有关系，注册复制子对象并广播移入
	Item = InItem;
	RegisterItemSubObject();
	OnItemBoundEvent.Broadcast(InItem);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] BindItem：物品 %s(%s) 绑定成功"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(InItem),
		*GetNameSafe(InItem->GetClass())
	);
}

USingularisItem* USingularisItemComponent::TakeItem()
{
	// 1) 空状态安全返回
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Display, TEXT("[%s] TakeItem：无物品可取出"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 解除复制注册，广播取出并清空持有，将引用权交还调用方
	USingularisItem* const OutItem = Item.Get();
	UnregisterItemSubObject();
	OnItemReleasedEvent.Broadcast(OutItem);
	Item = nullptr;

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] TakeItem：物品 %s(%s) 取出成功"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OutItem),
		*GetNameSafe(OutItem->GetClass())
	);
	return OutItem;
}

void USingularisItemComponent::RegisterItemSubObject()
{
	if (Item == nullptr)
		return;

	if (GetOwner()->HasAuthority())
		AddReplicatedSubObject(Item.Get());
}

void USingularisItemComponent::UnregisterItemSubObject()
{
	if (Item == nullptr)
		return;

	if (GetOwner()->HasAuthority())
		RemoveReplicatedSubObject(Item.Get());
}
