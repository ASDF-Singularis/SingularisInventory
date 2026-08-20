#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisInventoryComponent.generated.h"

class UDataTable;
class AActor;
class USingularisItem;
class USingularisPocketComponent;

/**
 * 引力奇点物库存组件。
 *
 * 生成器与联动协调器：玩家身上物品生命周期的单点入口（SSOT）。
 * 非容器——容器是 USingularisPocketComponent 及未来背包等具体类；本组件负责
 * 物品进入 / 离开世界、容器间流转的协调。
 *
 * 不变式：突变 API（SpawnItemInWorld / CollectItem）由服务端权威执行，
 * BlueprintAuthorityOnly 强制；客户端不应直接调用。
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

	/** 物品静态数据与形态映射的数据表，行类型 FSingularisMagicalElementRow。 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物库存|参数",
		meta = (
			DisplayName = "物品数据表",
			RequiredAssetDataTags = "RowStructure=/Script/SingularisInventory.SingularisMagicalElementRow"
		)
	)
	TObjectPtr<UDataTable> ItemTable = nullptr;

#pragma endregion

public:
#pragma region Constructors

	USingularisInventoryComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;

#pragma endregion

#pragma region API

	/**
	 * 将物品实例放入世界。
	 * 按物品类查数据表取形态 Actor 类 → SpawnActor 形态 Actor → 绑定 ItemComponent。
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

#pragma endregion

private:
#pragma region Internal Function

	/** 按物品实例的类在数据表中查其形态 Actor 类。 */
	TSubclassOf<AActor> FindFormActorClassForItem(USingularisItem* Item);

#pragma endregion
};
