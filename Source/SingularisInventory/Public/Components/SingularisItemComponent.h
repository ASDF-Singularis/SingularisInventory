#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisItemComponent.generated.h"

class USingularisItem;

#pragma region 委托签名

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemBoundSignature, USingularisItem*, Item);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemReleasedSignature, USingularisItem*, Item);

#pragma endregion

/**
 * 引力奇点物品组件。
 *
 * 挂载于物品在世界中的形态 Actor，承载并强持有 USingularisItem 物品实例。
 * 无任何配置：物品形态与物品定义的映射由库存子系统自动化构建，
 * 权威端 BeginPlay 阶段，若外部尚未填充物品实例，则以自身类反查物品定义并自动物化绑定
 * （地图放置与运行时生成的形态 Actor 均适用）；
 * 外部已填充物品实例（生成方调用 BindItem）时以既有实例为准，不再自动生成。
 * 容器收容、离开 UWorld 等场景由调用方调用 TakeItem 取出物品实例后再销毁形态 Actor。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物品组件")
)
class SINGULARISINVENTORY_API USingularisItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Event Dispatcher

	UPROPERTY(
		BlueprintAssignable,
		Category = "引力奇点物品组件|事件分发器",
		meta = (DisplayName = "物品移入")
	)
	FOnItemBoundSignature OnItemBoundEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "引力奇点物品组件|事件分发器",
		meta = (DisplayName = "物品取出")
	)
	FOnItemReleasedSignature OnItemReleasedEvent{};

#pragma endregion

private:
#pragma region State

	/**
	 * 当前持有的物品实例。
	 * 运行时由形态 Actor 生成方移入，容器收容时取出，不暴露给编辑器配置。
	 */
	UPROPERTY(Replicated, Transient, DuplicateTransient)
	TObjectPtr<USingularisItem> Item = nullptr;

#pragma endregion

public:
#pragma region Constructors

	USingularisItemComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion

#pragma region API

	/**
	 * 获取当前持有的物品实例。
	 * @return 当前持有的物品实例；空持有返回 nullptr
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "引力奇点物品组件|API",
		meta = (DisplayName = "获取物品实例")
	)
	USingularisItem* GetItem() const;

	/**
	 * 判断是否持有物品实例。
	 * @return 持有物品实例返回 true，否则返回 false
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "引力奇点物品组件|API",
		meta = (DisplayName = "是否持有物品")
	)
	bool HasItem() const;

	/**
	 * 将物品实例移入组件，建立强持有关系。
	 * 若组件已持有其他物品实例，先解除旧引用并广播取出事件，再绑定新实例。
	 * 幂等：重复绑定同一实例无副作用；空指针入参直接忽略。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "引力奇点物品组件|API",
		meta = (DisplayName = "移入物品")
	)
	void BindItem(USingularisItem* InItem);

	/**
	 * 取出当前持有的物品实例，解除持有关系并将引用权交还调用方。
	 * 调用方负责在销毁形态 Actor 前调用本函数以取回物品实例。
	 * 幂等：空状态下调用安全返回 nullptr。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "引力奇点物品组件|API",
		meta = (DisplayName = "取出物品")
	)
	USingularisItem* TakeItem();

	/**
	 * 清空并放弃当前持有的物品实例：解除复制注册与强持有并广播取出事件，不返回实例。
	 * 用于丢出后不可再拾取等场景；自动生成仅在 BeginPlay 发生一次，清理后形态即长期处于空持有状态。
	 * 幂等：空状态下调用安全无副作用。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "引力奇点物品组件|API",
		meta = (DisplayName = "清除物品")
	)
	void ClearItem();

#pragma endregion

private:
#pragma region Internal Function

	/** 将 Item 及其片段注册为网络复制子对象，仅在权威端执行。 */
	void RegisterItemSubObject();

	/** 将 Item 及其片段从网络复制列表移除，仅在权威端执行。 */
	void UnregisterItemSubObject();

#pragma endregion
};
