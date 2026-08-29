#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <InputActionValue.h>
#include <Components/ActorComponent.h>

#include "SingularisItemActionComponent.generated.h"

class USingularisItem;

/**
 * 引力奇点物品动作组件。
 *
 * 挂载于 Character，作为物品动作的执行器：经物品查询子系统查数据表取动作映射，
 * 从 Owner 推导动作上下文（Controller / Instigator / Avatar），
 * 按动作标签层级匹配命中管线并逐动作执行。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物品动作组件")
)
class SINGULARISINVENTORY_API USingularisItemActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Constructors

	USingularisItemActionComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;

#pragma endregion

#pragma region API

	/**
	 * 尝试按动作标签执行物品动作管线。
	 * 查数据表取动作映射，从 Owner 推导动作上下文，逐动作执行。
	 * @param Item 目标物品实例
	 * @param ActionTag 动作标签
	 * @param InputValue 触发输入值
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品动作|API",
		meta = (DisplayName = "尝试执行动作")
	)
	void TryAction(USingularisItem* Item, const FGameplayTag& ActionTag, const FInputActionValue& InputValue);

#pragma endregion
};
