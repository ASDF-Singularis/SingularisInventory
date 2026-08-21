#pragma once

#include <CoreMinimal.h>
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
struct FInputActionValue;

/**
 * 引力奇点物库存组件（调度器）。
 *
 * 可选的开箱即用调度器，挂在 PlayerController 上：主动查询所控 Character 上的口袋等容器，
 * 规划易用 API（拾取 / 丢弃），并一条管输入（选中本地、丢弃走服务端 RPC）。
 * 下层 PocketComponent / ItemComponent / 查询子系统保持独立可用，开发者可不用本调度器自行实现。
 *
 * 不变式：突变 API（SpawnItemInWorld / CollectItem / PickupItem / DropSelectedItem）
 * 服务端权威，BlueprintAuthorityOnly 强制；选中为本地行为，不经服务端。
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
		meta = (DisplayName = "输入映射上下文")
	)
	TObjectPtr<UInputMappingContext> InputMappingContext = nullptr;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "输入优先级")
	)
	int32 InputPriority = 10;

	/** 选中插槽输入动作数组，索引即插槽号（数字小键盘 1..N 映射到 0..N-1）。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "选中插槽输入动作")
	)
	TArray<TObjectPtr<UInputAction>> SelectSlotActions{};

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|输入",
		meta = (DisplayName = "丢弃输入动作")
	)
	TObjectPtr<UInputAction> DropInputAction = nullptr;

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
	 * 将物品实例放入世界。
	 * 经 USingularisInventoryItemSubsystem 查物品形态 Actor 类 → SpawnActor 形态 Actor → 绑定 ItemComponent。
	 * 形态 Actor 蓝图未预先挂 ItemComponent 时，物品仅进入世界、不可收容。
	 * 调用方须确保物品实例已从原持有方（如容器插槽）取出，避免重复持有。
	 * @return 形态 Actor；物品类未在表中、查表失败或生成失败返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "生成物品入世界")
	)
	AActor* SpawnItemInWorld(USingularisItem* Item, FTransform Transform);

	/**
	 * 从世界收容物品到容器。
	 * 内部查找形态 Actor 上的 ItemComponent → TakeItem 取回实例 → Destroy 形态 Actor →
	 * 提供目标容器则入容器，否则返回实例由调用方处置。
	 * @return 收容后的物品实例；形态 Actor 无 ItemComponent 或无物品、入参非法返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "收容物品出世界")
	)
	USingularisItem* CollectItem(AActor* FormActor, USingularisPocketComponent* TargetContainer = nullptr);

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
	 * 丢弃指定插槽物品入世界（角色前方）。
	 * RemoveItemAt 取出（口袋 relinquish 持有）→ SpawnItemInWorld 生成入世界。
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "丢弃物品")
	)
	void DropSelectedItem(int32 SlotIndex);

#pragma endregion

private:
#pragma region RPC

	UFUNCTION(Server, Reliable)
	void Server_DropSelectedItem(int32 SlotIndex);

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

	UFUNCTION()
	void OnPossessPawnChanged(APawn* OldPawn, APawn* NewPawn) const;

#pragma endregion
};
