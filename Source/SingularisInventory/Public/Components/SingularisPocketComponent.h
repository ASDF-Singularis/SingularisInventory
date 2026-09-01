#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "Types/SingularisPocketType.h"
#include "SingularisPocketComponent.generated.h"

class USingularisItem;
class USingularisItemDefinition;

#pragma region 委托签名

/** 物品加入指定插槽。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAddedSignature, int32, SlotIndex, USingularisItem*, Item);

/** 物品从指定插槽移除。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemovedSignature, int32, SlotIndex, USingularisItem*, Item);

/** 选中插槽变化。空选 / 选空槽均合法，仅当索引变化时触发。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectionChangedSignature, int32, OldSlotIndex, int32, NewSlotIndex);

/** 选中物品变化（选中物品即手持物品）。选中索引变化或选中槽内物品变化时触发，供装备 / 手持系统观察。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSelectedItemChangedSignature,
	USingularisItem*,
	OldItem,
	USingularisItem*,
	NewItem
);

/** 两个插槽的物品发生交换。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemsSwappedSignature, int32, SlotIndexA, int32, SlotIndexB);

/** 口袋占用状态（空 / 部分 / 满）变化。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPocketOccupancyChangedSignature,
	ESingularisPocketOccupancy,
	OldState,
	ESingularisPocketOccupancy,
	NewState
);

#pragma endregion

/**
 * 引力奇点口袋组件。
 *
 * 在固定容量的插槽数组中持有若干 USingularisItem 实例，并维护一个"选中插槽"状态
 * 以承担手持物品职责：选中插槽即手持目标，选中空槽即空手。
 *
 * 物品的进 / 出由调用方通过 AddItem / RemoveItem* 触发，内部自动管理网络复制子对象注册。
 * 事件按原子职责拆分：OnItemAdded / OnItemRemoved / OnSelectionChanged 为槽位与选中索引变化，
 * OnSelectedItemChanged 为手持物品变化（选中物品即手持物品），OnItemsSwapped / OnPocketOccupancyChanged
 * 为交换与占用状态边界，观察者按需订阅、互不干扰。
 *
 * 权威端：API 内直接触发事件。
 * 远程客户端：通过 OnRep 与上一帧快照 diff 触发等价事件，避免双触发。
 *
 * 不变式：突变 API（AddItem / RemoveItem* / SelectSlot* / SwapSlots / Clear）
 * 仅在权威端调用；客户端经 OnRep 获得事件，不应直接调用。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点口袋组件")
)
class SINGULARISINVENTORY_API USingularisPocketComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Parameter

	/** 口袋最大插槽数，运行时不可变更。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋|参数",
		meta = (DisplayName = "容量", ClampMin = "1")
	)
	int32 Capacity = 4;

	/**
	 * 口袋在 BeginPlay 阶段自动物化并放入对应索引插槽的初始物品定义数组。
	 * 数组索引对应插槽索引；超出 Capacity 的元素忽略，空元素对应插槽留空。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋组件|参数",
		meta = (DisplayName = "初始物品定义")
	)
	TArray<TObjectPtr<USingularisItemDefinition>> InitialDefinitions{};

#pragma endregion

#pragma region 事件分发器

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点口袋|事件分发器",
		meta = (DisplayName = "物品加入")
	)
	FOnItemAddedSignature OnItemAddedEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点口袋|事件分发器",
		meta = (DisplayName = "物品移除")
	)
	FOnItemRemovedSignature OnItemRemovedEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点口袋|事件分发器",
		meta = (DisplayName = "选中变化")
	)
	FOnSelectionChangedSignature OnSelectionChangedEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点口袋|事件分发器",
		meta = (DisplayName = "选中物品变化")
	)
	FOnSelectedItemChangedSignature OnSelectedItemChangedEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点口袋|事件分发器",
		meta = (DisplayName = "插槽交换")
	)
	FOnItemsSwappedSignature OnItemsSwappedEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点口袋|事件分发器",
		meta = (DisplayName = "占用状态变化")
	)
	FOnPocketOccupancyChangedSignature OnPocketOccupancyChangedEvent{};

#pragma endregion

private:
#pragma region Internal Variable

	/** 插槽数组，长度恒等于 Capacity。 */
	UPROPERTY(ReplicatedUsing = OnRep_Slots, Transient, DuplicateTransient)
	TArray<FSingularisPocketSlot> Slots{};

	/** 当前选中插槽索引，INDEX_NONE 表示无选中。客户端本地状态，不复制（选中为本地行为）。 */
	int32 SelectedSlotIndex = INDEX_NONE;

	/** 客户端 OnRep diff 用的上一帧插槽数组快照，非复制。 */
	UPROPERTY(Transient)
	TArray<FSingularisPocketSlot> PreviousSlotsSnapshot{};

	/** 上一帧占用状态缓存，用于幂等边界检测，非复制。 */
	ESingularisPocketOccupancy PreviousOccupancyState = ESingularisPocketOccupancy::Empty;

#pragma endregion

public:
#pragma region Constructors

	USingularisPocketComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion

#pragma region State

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "是否为空")
	)
	bool IsEmpty() const;

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "是否已满")
	)
	bool IsFull() const;

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "获取指定插槽物品")
	)
	USingularisItem* GetItem(int32 SlotIndex) const;

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "获取选中索引")
	)
	int32 GetSelectedIndex() const { return SelectedSlotIndex; }

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "是否有选中")
	)
	bool HasSelection() const { return SelectedSlotIndex != INDEX_NONE; }

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "获取选中物品")
	)
	USingularisItem* GetSelectedItem() const;

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点口袋|State",
		meta = (DisplayName = "获取占用状态")
	)
	ESingularisPocketOccupancy GetOccupancyState() const;

#pragma endregion

#pragma region API

	/**
	 * 自动寻找首个空插槽放入物品。
	 * 幂等：若物品已存在于此口袋，直接返回其所在插槽。
	 * @return 放入的插槽索引；口袋已满或入参非法返回 INDEX_NONE
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "加入物品")
	)
	int32 AddItem(USingularisItem* Item);

	/**
	 * 将物品放入指定插槽。
	 * 目标插槽必须为空，且该物品未存在于其他插槽。
	 * @return 是否成功放入
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "加入物品至指定插槽")
	)
	bool AddItemAt(USingularisItem* Item, int32 SlotIndex);

	/**
	 * 按物品指针查找并移除。
	 * @return 是否成功移除
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "移除物品")
	)
	bool RemoveItem(USingularisItem* Item);

	/**
	 * 移除指定插槽的物品并返还实例，便于转移至容器 / 形态 Actor。
	 * @return 被取出的物品实例；插槽为空或索引非法返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "移除指定插槽物品")
	)
	USingularisItem* RemoveItemAt(int32 SlotIndex);

	/**
	 * 移除选中插槽内的物品并返还实例，便于转移至容器 / 形态 Actor。
	 * 移除后选中索引保持不变（选中空槽即空手）。
	 * @return 被取出的物品实例；无选中或选中槽为空返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "移除选中物品")
	)
	USingularisItem* RemoveSelectedItem();

	/**
	 * 设置选中插槽。INDEX_NONE 清空选中；指向空槽即空手。
	 * 幂等：与当前选中相同时无副作用。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "选中插槽")
	)
	void SelectSlot(int32 SlotIndex);

	/** 循环向后选中下一插槽。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "选中下一")
	)
	void SelectNext();

	/** 循环向前选中上一插槽。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "选中上一")
	)
	void SelectPrevious();

	/** 交换两个插槽内的物品。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "交换插槽")
	)
	void SwapSlots(int32 SlotIndexA, int32 SlotIndexB);

	/** 清空全部插槽，依次触发 OnItemRemovedEvent。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋|API",
		meta = (DisplayName = "清空口袋")
	)
	void Clear();

#pragma endregion

private:
#pragma region Response

	/** 客户端 Slots 复制回调，diff 快照触发等价的加入 / 移除事件。 */
	UFUNCTION()
	void OnRep_Slots();

#pragma endregion

#pragma region Internal Function

	/** 按 Capacity 预分配空插槽。 */
	void InitializeSlots();

	/** 注册指定插槽物品为网络复制子对象，仅在权威端执行。 */
	void RegisterSlotSubObject(int32 SlotIndex);

	/** 注销指定插槽物品的复制子对象，仅在权威端执行。 */
	void UnregisterSlotSubObject(int32 SlotIndex);

	/** 注销全部插槽物品的复制子对象。 */
	void UnregisterAllSubObjects();

	/** 查询首个空插槽索引，无返回 INDEX_NONE。 */
	int32 FindFirstEmptySlot() const;

	/** 按物品指针查找所在插槽索引，无返回 INDEX_NONE。 */
	int32 FindSlotOfItem(const USingularisItem* Item) const;

	/** 索引合法性校验。 */
	bool IsValidSlotIndex(int32 SlotIndex) const;

	/**
	 * 广播单个插槽由 OldItem → NewItem 的原子过渡事件。
	 * Old == New 时无副作用。
	 */
	void BroadcastSlotTransition(int32 SlotIndex, USingularisItem* OldItem, USingularisItem* NewItem) const;

	/** 将 Prev / Curr 两份插槽数组 diff，逐插槽触发过渡事件并更新 Prev 快照。 */
	void DiffAndBroadcastSlots();

	/** 计算当前占用状态，与缓存比较，仅在状态位变化时广播事件并更新缓存。 */
	void BroadcastOccupancyChangeIfChanged();

	/** 默认选中首个插槽，延迟到下一帧执行确保所有订阅者完成 BeginPlay 绑定。 */
	void InitializeDefaultSelection();

#pragma endregion
};
