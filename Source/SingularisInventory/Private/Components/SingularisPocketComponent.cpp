#include "Components/SingularisPocketComponent.h"

#include <Net/UnrealNetwork.h>

#include "SingularisInventory.h"
#include "Objects/SingularisItem.h"

USingularisPocketComponent::USingularisPocketComponent()
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisPocketComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1) 仅权威端预分配插槽数组；客户端由复制同步，避免本地写入复制属性
	if (GetOwner()->HasAuthority())
	{
		InitializeSlots();
		UE_LOG(LogSingularisInventory, Display, TEXT("[%s] 口袋插槽初始化完成，容量 %d"), *GetNameSafe(GetOwner()), Capacity);

		// 2) 设计期模板物化：按索引对应插槽顺序，物化 InitialItems 中的模板实例
		for (auto i = 0; i < InitialItems.Num() && i < Slots.Num(); ++i)
		{
			USingularisItem* const Template = InitialItems[i];
			if (!IsValid(Template))
				continue;

			USingularisItem* const Materialized = USingularisItem::MaterializeFromTemplate(GetWorld(), Template);
			if (!IsValid(Materialized))
				continue;

			AddItemAt(Materialized, i);
		}
	}

	// 3) 建立客户端 OnRep diff 的初始基线快照
	PreviousSlotsSnapshot = Slots;

	// 4) 基线化占用状态缓存，初始状态由观察者主动拉取
	PreviousOccupancyState = GetOccupancyState();

	// 5) 默认选中首个插槽：延迟到下一帧执行，确保所有订阅者完成 BeginPlay 事件绑定
	if (Capacity > 0)
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::InitializeDefaultSelection);
}

void USingularisPocketComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1) 防御性清理：解除全部复制子对象注册，避免悬挂引用
	UnregisterAllSubObjects();

	Super::EndPlay(EndPlayReason);
}

void USingularisPocketComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(USingularisPocketComponent, Slots, COND_OwnerOnly);
}

bool USingularisPocketComponent::IsEmpty() const
{
	for (const FSingularisPocketSlot& Slot : Slots)
	{
		if (!Slot.IsEmpty())
			return false;
	}
	return true;
}

bool USingularisPocketComponent::IsFull() const
{
	return Slots.Num() == Capacity && FindFirstEmptySlot() == INDEX_NONE;
}

USingularisItem* USingularisPocketComponent::GetItem(const int32 SlotIndex) const
{
	if (!IsValidSlotIndex(SlotIndex))
		return nullptr;
	return Slots[SlotIndex].Item;
}

USingularisItem* USingularisPocketComponent::GetSelectedItem() const
{
	return HasSelection() ? GetItem(SelectedSlotIndex) : nullptr;
}

ESingularisPocketOccupancy USingularisPocketComponent::GetOccupancyState() const
{
	// 1) 空与满互斥，其余为部分占用
	if (IsEmpty())
		return ESingularisPocketOccupancy::Empty;
	if (IsFull())
		return ESingularisPocketOccupancy::Full;
	return ESingularisPocketOccupancy::Partial;
}

int32 USingularisPocketComponent::AddItem(USingularisItem* Item)
{
	// 1) 零信任校验：空入参直接忽略
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] AddItem：物品实例为空"), *GetNameSafe(GetOwner()));
		return INDEX_NONE;
	}

	// 2) 插槽与容量一致性校验：运行时修改 Capacity 需重新初始化（仅权威端校验）
	if (GetOwner()->HasAuthority() && !ensureMsgf(
		Slots.Num() == Capacity,
		TEXT("[%s] AddItem：插槽数 %d 与容量 %d 失配"),
		*GetNameSafe(GetOwner()),
		Slots.Num(),
		Capacity
	))
		return INDEX_NONE;

	// 3) 幂等：物品已存在于此口袋，直接返回其所在插槽
	const int32 ExistingSlot = FindSlotOfItem(Item);
	if (ExistingSlot != INDEX_NONE)
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] AddItem：物品 %s 已在插槽 %d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item),
			ExistingSlot
		);
		return ExistingSlot;
	}

	// 4) 寻找首个空插槽放入，注册复制子对象并广播原子过渡
	const int32 TargetSlot = FindFirstEmptySlot();
	if (TargetSlot == INDEX_NONE)
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] AddItem：口袋已满，物品 %s 未放入"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item)
		);
		return INDEX_NONE;
	}

	Slots[TargetSlot].Item = Item;
	RegisterSlotSubObject(TargetSlot);
	BroadcastSlotTransition(TargetSlot, nullptr, Item);
	BroadcastOccupancyChangeIfChanged();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] AddItem：物品 %s(%s) 放入插槽 %d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass()),
		TargetSlot
	);
	return TargetSlot;
}

bool USingularisPocketComponent::AddItemAt(USingularisItem* Item, const int32 SlotIndex)
{
	// 1) 零信任校验：空入参或非法索引直接失败
	if (Item == nullptr || !IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] AddItemAt：入参非法（物品 %s，索引 %d）"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item),
			SlotIndex
		);
		return false;
	}

	// 2) 目标插槽必须为空，且物品未存在于其他插槽
	if (!Slots[SlotIndex].IsEmpty())
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] AddItemAt：插槽 %d 已被占用"), *GetNameSafe(GetOwner()), SlotIndex);
		return false;
	}
	if (FindSlotOfItem(Item) != INDEX_NONE)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] AddItemAt：物品 %s 已在插槽 %d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item),
			FindSlotOfItem(Item)
		);
		return false;
	}

	Slots[SlotIndex].Item = Item;
	RegisterSlotSubObject(SlotIndex);
	BroadcastSlotTransition(SlotIndex, nullptr, Item);
	BroadcastOccupancyChangeIfChanged();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] AddItemAt：物品 %s(%s) 放入插槽 %d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass()),
		SlotIndex
	);
	return true;
}

bool USingularisPocketComponent::RemoveItem(USingularisItem* Item)
{
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] RemoveItem：物品实例为空"), *GetNameSafe(GetOwner()));
		return false;
	}

	const int32 TargetSlot = FindSlotOfItem(Item);
	if (TargetSlot == INDEX_NONE)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] RemoveItem：物品 %s 不在口袋中"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item)
		);
		return false;
	}

	return RemoveItemAt(TargetSlot) != nullptr;
}

USingularisItem* USingularisPocketComponent::RemoveItemAt(const int32 SlotIndex)
{
	// 1) 索引合法性校验
	if (!IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] RemoveItemAt：索引 %d 非法"),
			*GetNameSafe(GetOwner()),
			SlotIndex
		);
		return nullptr;
	}

	// 2) 空插槽安全返回
	if (Slots[SlotIndex].IsEmpty())
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] RemoveItemAt：插槽 %d 为空"),
			*GetNameSafe(GetOwner()),
			SlotIndex
		);
		return nullptr;
	}

	// 3) 解除复制注册，清空插槽，广播原子过渡，返还物品实例
	USingularisItem* const OldItem = Slots[SlotIndex].Item;
	UnregisterSlotSubObject(SlotIndex);
	Slots[SlotIndex].Item = nullptr;
	BroadcastSlotTransition(SlotIndex, OldItem, nullptr);
	BroadcastOccupancyChangeIfChanged();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] RemoveItemAt：物品 %s(%s) 移出插槽 %d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(OldItem),
		*GetNameSafe(OldItem->GetClass()),
		SlotIndex
	);
	return OldItem;
}

USingularisItem* USingularisPocketComponent::RemoveSelectedItem()
{
	// 1) 无选中即无可移除物品
	if (!HasSelection())
	{
		UE_LOG(LogSingularisInventory, Display, TEXT("[%s] RemoveSelectedItem：无选中插槽"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 复用 RemoveItemAt 的合法性校验、复制子对象注销与原子过渡广播
	return RemoveItemAt(SelectedSlotIndex);
}

void USingularisPocketComponent::SelectSlot(const int32 SlotIndex)
{
	// 1) 合法值：INDEX_NONE 清空选中，或有效插槽索引；其余忽略
	if (SlotIndex != INDEX_NONE && !IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SelectSlot：索引 %d 非法"), *GetNameSafe(GetOwner()), SlotIndex);
		return;
	}

	// 2) 幂等：与当前选中相同则无副作用
	if (SlotIndex == SelectedSlotIndex)
	{
		UE_LOG(LogSingularisInventory, Display, TEXT("[%s] SelectSlot：选中未变化（%d）"), *GetNameSafe(GetOwner()), SlotIndex);
		return;
	}

	const int32 OldSlotIndex = SelectedSlotIndex;
	SelectedSlotIndex = SlotIndex;
	OnSelectionChangedEvent.Broadcast(OldSlotIndex, SlotIndex);

	// 3) 携带新旧选中物品广播手持变化（选中物品即手持物品）；守卫幂等：空槽切空槽不触发
	USingularisItem* const OldItem = GetItem(OldSlotIndex);
	USingularisItem* const NewItem = GetItem(SlotIndex);
	if (OldItem != NewItem)
		OnSelectedItemChangedEvent.Broadcast(OldItem, NewItem);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] SelectSlot：选中 %d → %d"),
		*GetNameSafe(GetOwner()),
		OldSlotIndex,
		SlotIndex
	);
}

void USingularisPocketComponent::SelectNext()
{
	if (Capacity <= 0)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SelectNext：容量 %d 无效"), *GetNameSafe(GetOwner()), Capacity);
		return;
	}

	const int32 Base = SelectedSlotIndex == INDEX_NONE ? 0 : SelectedSlotIndex;
	const int32 Target = (Base + 1) % Capacity;
	SelectSlot(Target);
}

void USingularisPocketComponent::SelectPrevious()
{
	if (Capacity <= 0)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SelectPrevious：容量 %d 无效"),
			*GetNameSafe(GetOwner()),
			Capacity
		);
		return;
	}

	const int32 Base = SelectedSlotIndex == INDEX_NONE ? Capacity - 1 : SelectedSlotIndex;
	const int32 Target = (Base - 1 + Capacity) % Capacity;
	SelectSlot(Target);
}

void USingularisPocketComponent::SwapSlots(const int32 SlotIndexA, const int32 SlotIndexB)
{
	// 1) 两端均合法且不同才交换
	if (!IsValidSlotIndex(SlotIndexA) || !IsValidSlotIndex(SlotIndexB) || SlotIndexA == SlotIndexB)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SwapSlots：索引非法（%d ↔ %d）"),
			*GetNameSafe(GetOwner()),
			SlotIndexA,
			SlotIndexB
		);
		return;
	}

	// 2) 交换物品引用，物品仍在口袋内无需调整复制子对象注册
	USingularisItem* const OldA = Slots[SlotIndexA].Item;
	USingularisItem* const OldB = Slots[SlotIndexB].Item;
	Slots[SlotIndexA].Item = OldB;
	Slots[SlotIndexB].Item = OldA;

	// 3) 逐插槽广播原子过渡
	BroadcastSlotTransition(SlotIndexA, OldA, OldB);
	BroadcastSlotTransition(SlotIndexB, OldB, OldA);

	// 4) 广播交换事件，供观察者区分"交换"与"先移除再加入"
	OnItemsSwappedEvent.Broadcast(SlotIndexA, SlotIndexB);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] SwapSlots：插槽 %d ↔ %d 交换完成"),
		*GetNameSafe(GetOwner()),
		SlotIndexA,
		SlotIndexB
	);
}

void USingularisPocketComponent::Clear()
{
	auto ClearedCount = 0;

	for (auto i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].IsEmpty())
			continue;

		USingularisItem* const OldItem = Slots[i].Item;
		UnregisterSlotSubObject(i);
		Slots[i].Item = nullptr;
		BroadcastSlotTransition(i, OldItem, nullptr);
		++ClearedCount;
	}

	// 2) 广播清空可能触发的占用状态边界变化
	BroadcastOccupancyChangeIfChanged();

	UE_LOG(LogSingularisInventory, Display, TEXT("[%s] Clear：清空 %d 个插槽"), *GetNameSafe(GetOwner()), ClearedCount);
}

void USingularisPocketComponent::OnRep_Slots()
{
	// 权威端由 API 直接触发事件，跳过 diff 避免双触发
	if (GetOwner()->HasAuthority())
		return;

	DiffAndBroadcastSlots();
}

void USingularisPocketComponent::InitializeSlots()
{
	Slots.SetNum(Capacity);
}

void USingularisPocketComponent::RegisterSlotSubObject(const int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex))
		return;

	USingularisItem* const Item = Slots[SlotIndex].Item;
	if (Item == nullptr)
		return;

	if (GetOwner()->HasAuthority())
		AddReplicatedSubObject(Item);
}

void USingularisPocketComponent::UnregisterSlotSubObject(const int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex))
		return;

	USingularisItem* const Item = Slots[SlotIndex].Item;
	if (Item == nullptr)
		return;

	if (GetOwner()->HasAuthority())
		RemoveReplicatedSubObject(Item);
}

void USingularisPocketComponent::UnregisterAllSubObjects()
{
	for (auto i = 0; i < Slots.Num(); ++i)
		UnregisterSlotSubObject(i);
}

int32 USingularisPocketComponent::FindFirstEmptySlot() const
{
	for (auto i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].IsEmpty())
			return i;
	}
	return INDEX_NONE;
}

int32 USingularisPocketComponent::FindSlotOfItem(const USingularisItem* Item) const
{
	if (Item == nullptr)
		return INDEX_NONE;

	for (auto i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Item == Item)
			return i;
	}
	return INDEX_NONE;
}

bool USingularisPocketComponent::IsValidSlotIndex(const int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < Slots.Num();
}

void USingularisPocketComponent::BroadcastSlotTransition(
	const int32 SlotIndex,
	USingularisItem* OldItem,
	USingularisItem* NewItem
) const
{
	// 1) 无变化无副作用
	if (OldItem == NewItem)
		return;

	// 2) 槽位原子过渡：既有又新（交换）先移除后加入
	if (OldItem != nullptr && NewItem != nullptr)
	{
		OnItemRemovedEvent.Broadcast(SlotIndex, OldItem);
		OnItemAddedEvent.Broadcast(SlotIndex, NewItem);
	}
	else if (OldItem != nullptr)
		OnItemRemovedEvent.Broadcast(SlotIndex, OldItem);
	else
		OnItemAddedEvent.Broadcast(SlotIndex, NewItem);

	// 3) 选中槽内物品变化即手持变化（选中物品即手持物品）
	if (SlotIndex == SelectedSlotIndex)
		OnSelectedItemChangedEvent.Broadcast(OldItem, NewItem);
}

void USingularisPocketComponent::DiffAndBroadcastSlots()
{
	// 1) 取较大长度，对越界端按空槽处理
	const int32 MaxNum = FMath::Max(Slots.Num(), PreviousSlotsSnapshot.Num());

	for (auto i = 0; i < MaxNum; ++i)
	{
		USingularisItem* const OldItem = i < PreviousSlotsSnapshot.Num() ? PreviousSlotsSnapshot[i].Item : nullptr;
		USingularisItem* const NewItem = i < Slots.Num() ? Slots[i].Item : nullptr;
		BroadcastSlotTransition(i, OldItem, NewItem);
	}

	// 2) 更新基线快照
	PreviousSlotsSnapshot = Slots;

	// 3) 广播占用状态边界变化
	BroadcastOccupancyChangeIfChanged();
}

void USingularisPocketComponent::BroadcastOccupancyChangeIfChanged()
{
	// 1) 计算新状态，与缓存比较，状态位未变化则无副作用
	const ESingularisPocketOccupancy NewState = GetOccupancyState();
	if (NewState == PreviousOccupancyState)
		return;

	// 2) 更新缓存并广播边界变化
	const ESingularisPocketOccupancy OldState = PreviousOccupancyState;
	PreviousOccupancyState = NewState;
	OnPocketOccupancyChangedEvent.Broadcast(OldState, NewState);
}

void USingularisPocketComponent::InitializeDefaultSelection()
{
	// 1) 容量无效则忽略，复用 SelectSlot 的幂等检查、广播与日志
	if (Capacity <= 0)
		return;

	// 2) 仅当尚未选中时才设置默认选中，避免覆盖运行时已设置的选中状态
	if (SelectedSlotIndex != INDEX_NONE)
		return;

	SelectSlot(0);
}
