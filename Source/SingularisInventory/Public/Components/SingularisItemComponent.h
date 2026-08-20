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
 * 形态 Actor 生成时由生成方调用 BindItem 将物品实例移入；容器收容、离开 UWorld
 * 等场景由调用方调用 TakeItem 取出物品实例后再销毁形态 Actor。
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
#pragma region 事件分发器

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点物品|事件分发器",
		meta = (DisplayName = "物品移入")
	)
	FOnItemBoundSignature OnItemBoundEvent{};

	UPROPERTY(
		BlueprintAssignable,
		Category = "SingularisInventory|引力奇点物品|事件分发器",
		meta = (DisplayName = "物品取出")
	)
	FOnItemReleasedSignature OnItemReleasedEvent{};

#pragma endregion

private:
#pragma region Internal Variable

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

#pragma region State

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物品|State",
		meta = (DisplayName = "获取物品实例")
	)
	USingularisItem* GetItem() const;

	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物品|State",
		meta = (DisplayName = "是否持有物品")
	)
	bool HasItem() const;

#pragma endregion

#pragma region API

	/**
	 * 将物品实例移入组件，建立强持有关系。
	 * 若组件已持有其他物品实例，先解除旧引用并广播取出事件，再绑定新实例。
	 * 幂等：重复绑定同一实例无副作用；空指针入参直接忽略。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品|API",
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
		Category = "SingularisInventory|引力奇点物品|API",
		meta = (DisplayName = "取出物品")
	)
	USingularisItem* TakeItem();

#pragma endregion

private:
#pragma region Internal Function

	/** 将 Item 注册为网络复制子对象，仅在权威端执行。 */
	void RegisterItemSubObject();

	/** 将 Item 从网络复制列表移除，仅在权威端执行。 */
	void UnregisterItemSubObject();

#pragma endregion
};
