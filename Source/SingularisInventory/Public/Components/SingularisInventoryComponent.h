#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <InputActionValue.h>
#include <Components/ActorComponent.h>

#include "SingularisInventoryComponent.generated.h"

class APlayerController;
class ACharacter;
class AActor;
class APawn;
class USingularisItem;
class USingularisPocketComponent;
class UInputMappingContext;
class UInputAction;

/**
 * 引力奇点物品片段输入。
 * 隶属本调度器（外部可整体替换 / 更名 / 移动），不属于片段域类型。
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisItemFragmentInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UInputAction> InputAction = nullptr;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		meta = (
			Categories = "Singularis.Inventory.Fragment",
			ForceSelection = "true"
		)
	)
	FGameplayTag FragmentTag{};
};

/**
 * 引力奇点物库存组件（调度器）。
 *
 * 可选的开箱即用调度器，挂在 PlayerController 上：主动查询所控 Character 上的口袋等容器，
 * 规划易用 API（拾取 / 丢弃），并一条管输入（选中本地、丢弃走服务端 RPC）。
 * 下层 PocketComponent / ItemComponent / 查询子系统保持独立可用，开发者可不用本调度器自行实现。
 *
 * 不变式：突变 API（SpawnItemInWorld / CollectItem / PickupItem / DropItem）
 * 服务端权威，BlueprintAuthorityOnly 强制；DropHeldItem 为客户端入口（读本地手持 → RPC）；选中为本地行为，不经服务端。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物库存组件")
)
class SINGULARISINVENTORY_API USingularisInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Parameter

	/** 丢弃距离（角色前方）。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|参数",
		meta = (DisplayName = "丢弃距离")
	)
	float DropDistance = 150.0f;

	/** 丢弃高度偏移。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|参数",
		meta = (DisplayName = "丢弃高度偏移")
	)
	float DropZOffset = 50.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "输入优先级")
	)
	int32 InputPriority = 10;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "输入映射上下文")
	)
	TObjectPtr<UInputMappingContext> InputMappingContext = nullptr;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "丢弃输入动作")
	)
	TObjectPtr<UInputAction> DropInputAction = nullptr;

	/** 物品片段输入集：输入动作与片段标签配对，标签路由到物品的片段。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "物品片段输入")
	)
	TArray<FSingularisItemFragmentInput> FragmentInputs{};

	/** 选中插槽输入动作数组，索引即插槽号（数字小键盘 1..N 映射到 0..N-1）。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "选中插槽输入动作")
	)
	TArray<TObjectPtr<UInputAction>> SelectSlotActions{};

#pragma endregion

private:
#pragma region Internal Variable

	TWeakObjectPtr<APlayerController> OwnerPlayerController = nullptr;

#pragma endregion

public:
#pragma region Constructors

	USingularisInventoryComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma endregion

#pragma region API

	/**
	 * 拾取世界物品入库存。
	 * CollectItem 收容出世界 → 按规则路由入容器（当前口袋优先；满则返回实例，未来扩展背包兜底）。
	 * @return 收容后的物品实例；入容器成功仍返回以便查询，未入容器时由调用方处置
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "拾取物品")
	)
	USingularisItem* PickupItem(AActor* FormActor);

	/**
	 * 丢弃指定物品入世界（角色前方）。
	 * Pocket->RemoveItem 取出（口袋 relinquish 持有）→ SpawnItemInWorld 生成入世界。
	 * 服务端原语；客户端经 Server_DropItem RPC 触发。
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "丢弃指定物品")
	)
	void DropItem(USingularisItem* Item);

	/**
	 * 丢弃手持物品入世界。
	 * 本地读所控口袋的选中物品 → 经 Server_DropItem RPC 上行服务端执行。
	 * 选中为本地行为，服务端不持有选中态，故丢弃手持须由客户端发起。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "丢弃手持物品")
	)
	void DropHeldItem();

	/**
	 * 触发手持物品的片段（客户端入口）。
	 * 本地读所控口袋的选中物品 → 经 Server_TriggerFragment RPC 上行服务端执行片段。
	 * 选中为本地行为，服务端不持有选中态，故触发须由客户端发起。
	 * @param FragmentTag 片段标签，路由到物品的片段
	 * @param InputValue 触发输入值
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "触发物品片段")
	)
	void TriggerFragment(const FGameplayTag& FragmentTag, const FInputActionValue& InputValue);

#pragma endregion

private:
#pragma region RPC

	UFUNCTION(Server, Reliable)
	void Server_DropItem(USingularisItem* Item);

	UFUNCTION(Server, Reliable)
	void Server_TriggerFragment(
		USingularisItem* Item,
		const FGameplayTag& FragmentTag,
		const FInputActionValue& InputValue
	);

#pragma endregion

#pragma region Internal Function

	/** 绑定 EnhancedInput 动作（选中 / 丢弃）。仅在本地控制器端绑定。 */
	void BindInputAction();

	/** 按所控 Character 是否存在增删输入映射上下文。 */
	void RefreshInputMappingContext() const;

	/** 取 OwnerPlayerController 所控 Character。 */
	ACharacter* GetControlledCharacter() const;

	/** 取所控 Character 上的口袋组件。 */
	USingularisPocketComponent* GetPocketComponent() const;

	/** 计算角色前方丢弃位置。 */
	FTransform ComputeDropTransform(const ACharacter* Character) const;

#pragma endregion

#pragma region Callback

	void HandleSelectSlot(const FInputActionValue& Value, int32 SlotIndex);
	void HandleDropInputAction(const FInputActionValue& Value);
	void HandleFragmentInput(const FInputActionValue& Value, FGameplayTag FragmentTag);

	UFUNCTION()
	void OnPossessPawnChanged(APawn* OldPawn, APawn* NewPawn) const;

#pragma endregion
};
