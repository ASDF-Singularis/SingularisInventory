#include "Components/SingularisPocketComponent.h"

#include <Net/UnrealNetwork.h>

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
		InitializeSlots();

	// 2) 建立客户端 OnRep diff 的初始基线快照
	PreviousSlotsSnapshot = Slots;
	PreviousSelectedSlotIndex = SelectedSlotIndex;
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
	DOREPLIFETIME_CONDITION(USingularisPocketComponent, SelectedSlotIndex, COND_OwnerOnly);
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

int32 USingularisPocketComponent::AddItem(USingularisItem* Item)
{
	// 1) 零信任校验：空入参直接忽略
	if (Item == nullptr)
		return INDEX_NONE;

	// 2) 幂等：物品已存在于此口袋，直接返回其所在插槽
	const int32 ExistingSlot = FindSlotOfItem(Item);
	if (ExistingSlot != INDEX_NONE)
		return ExistingSlot;

	// 3) 寻找首个空插槽放入，注册复制子对象并广播原子过渡
	const int32 TargetSlot = FindFirstEmptySlot();
	if (TargetSlot == INDEX_NONE)
		return INDEX_NONE;

	Slots[TargetSlot].Item = Item;
	RegisterSlotSubObject(TargetSlot);
	BroadcastSlotTransition(TargetSlot, nullptr, Item);

	return TargetSlot;
}

bool USingularisPocketComponent::AddItemAt(USingularisItem* Item, const int32 SlotIndex)
{
	// 1) 零信任校验：空入参或非法索引直接失败
	if (Item == nullptr || !IsValidSlotIndex(SlotIndex))
		return false;

	// 2) 目标插槽必须为空，且物品未存在于其他插槽
	if (!Slots[SlotIndex].IsEmpty())
		return false;
	if (FindSlotOfItem(Item) != INDEX_NONE)
		return false;

	Slots[SlotIndex].Item = Item;
	RegisterSlotSubObject(SlotIndex);
	BroadcastSlotTransition(SlotIndex, nullptr, Item);

	return true;
}

bool USingularisPocketComponent::RemoveItem(USingularisItem* Item)
{
	if (Item == nullptr)
		return false;

	const int32 TargetSlot = FindSlotOfItem(Item);
	if (TargetSlot == INDEX_NONE)
		return false;

	return RemoveItemAt(TargetSlot) != nullptr;
}

USingularisItem* USingularisPocketComponent::RemoveItemAt(const int32 SlotIndex)
{
	// 1) 索引合法性校验
	if (!IsValidSlotIndex(SlotIndex))
		return nullptr;

	// 2) 空插槽安全返回
	if (Slots[SlotIndex].IsEmpty())
		return nullptr;

	// 3) 解除复制注册，清空插槽，广播原子过渡，返还物品实例
	USingularisItem* const OldItem = Slots[SlotIndex].Item;
	UnregisterSlotSubObject(SlotIndex);
	Slots[SlotIndex].Item = nullptr;
	BroadcastSlotTransition(SlotIndex, OldItem, nullptr);

	return OldItem;
}

void USingularisPocketComponent::SelectSlot(const int32 SlotIndex)
{
	// 1) 合法值：INDEX_NONE 清空选中，或有效插槽索引；其余忽略
	if (SlotIndex != INDEX_NONE && !IsValidSlotIndex(SlotIndex))
		return;

	// 2) 幂等：与当前选中相同则无副作用
	if (SlotIndex == SelectedSlotIndex)
		return;

	const int32 OldSlotIndex = SelectedSlotIndex;
	SelectedSlotIndex = SlotIndex;
	OnSelectionChangedEvent.Broadcast(OldSlotIndex, SlotIndex);
}

void USingularisPocketComponent::SelectNext()
{
	if (Capacity <= 0)
		return;

	const int32 Base = SelectedSlotIndex == INDEX_NONE ? 0 : SelectedSlotIndex;
	const int32 Target = (Base + 1) % Capacity;
	SelectSlot(Target);
}

void USingularisPocketComponent::SelectPrevious()
{
	if (Capacity <= 0)
		return;

	const int32 Base = SelectedSlotIndex == INDEX_NONE ? Capacity - 1 : SelectedSlotIndex;
	const int32 Target = (Base - 1 + Capacity) % Capacity;
	SelectSlot(Target);
}

void USingularisPocketComponent::SwapSlots(const int32 SlotIndexA, const int32 SlotIndexB)
{
	// 1) 两端均合法且不同才交换
	if (!IsValidSlotIndex(SlotIndexA) || !IsValidSlotIndex(SlotIndexB) || SlotIndexA == SlotIndexB)
		return;

	// 2) 交换物品引用，物品仍在口袋内无需调整复制子对象注册
	USingularisItem* const OldA = Slots[SlotIndexA].Item;
	USingularisItem* const OldB = Slots[SlotIndexB].Item;
	Slots[SlotIndexA].Item = OldB;
	Slots[SlotIndexB].Item = OldA;

	// 3) 逐插槽广播原子过渡
	BroadcastSlotTransition(SlotIndexA, OldA, OldB);
	BroadcastSlotTransition(SlotIndexB, OldB, OldA);
}

void USingularisPocketComponent::Clear()
{
	for (auto i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].IsEmpty())
			continue;

		USingularisItem* const OldItem = Slots[i].Item;
		UnregisterSlotSubObject(i);
		Slots[i].Item = nullptr;
		BroadcastSlotTransition(i, OldItem, nullptr);
	}
}

void USingularisPocketComponent::OnRep_Slots()
{
	// 权威端由 API 直接触发事件，跳过 diff 避免双触发
	if (GetOwner()->HasAuthority())
		return;

	DiffAndBroadcastSlots();
}

void USingularisPocketComponent::OnRep_SelectedSlotIndex()
{
	if (GetOwner()->HasAuthority())
		return;

	if (PreviousSelectedSlotIndex == SelectedSlotIndex)
		return;

	OnSelectionChangedEvent.Broadcast(PreviousSelectedSlotIndex, SelectedSlotIndex);
	PreviousSelectedSlotIndex = SelectedSlotIndex;
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

	// 2) 既有又新：先移除后加入，保持原子顺序
	if (OldItem != nullptr && NewItem != nullptr)
	{
		OnItemRemovedEvent.Broadcast(SlotIndex, OldItem);
		OnItemAddedEvent.Broadcast(SlotIndex, NewItem);
		return;
	}

	// 3) 仅移除
	if (OldItem != nullptr)
	{
		OnItemRemovedEvent.Broadcast(SlotIndex, OldItem);
		return;
	}

	// 4) 仅加入
	OnItemAddedEvent.Broadcast(SlotIndex, NewItem);
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
}
