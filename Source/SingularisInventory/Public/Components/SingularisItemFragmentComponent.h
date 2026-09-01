#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <InputActionValue.h>
#include <Components/ActorComponent.h>

#include "SingularisItemFragmentComponent.generated.h"

class USingularisItem;

/**
 * 引力奇点物品片段组件。
 *
 * 挂载于 Character，作为物品片段的执行器：经物品实例背引用的定义取平铺片段数组，
 * 从 Owner 推导片段上下文（Controller / Instigator / Avatar），
 * 按片段自报标签层级匹配后逐片段执行。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物品片段组件")
)
class SINGULARISINVENTORY_API USingularisItemFragmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Constructors

	USingularisItemFragmentComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;

#pragma endregion

#pragma region API

	/**
	 * 按片段标签执行物品片段。
	 * 经物品实例背引用的定义取平铺片段数组，从 Owner 推导片段上下文，
	 * 按片段自报标签层级匹配后逐片段执行。
	 * @param Item 目标物品实例
	 * @param FragmentTag 片段标签
	 * @param InputValue 触发输入值
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品片段|API",
		meta = (DisplayName = "执行片段")
	)
	void Execute(USingularisItem* Item, const FGameplayTag& FragmentTag, const FInputActionValue& InputValue);

#pragma endregion
};
