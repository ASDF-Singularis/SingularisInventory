#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisInventoryComponent.generated.h"

class UDataTable;
class AActor;
class USingularisItem;
class USingularisItemComponent;
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

#pragma region Constructors

	USingularisInventoryComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;

#pragma endregion

#pragma region API

	/**
	 * 从数据表生成物品并放入世界。
	 * 查 Row → NewObject 物品实例（Outer 为瞬时包）→ SpawnActor 形态 Actor → 绑定 ItemComponent。
	 * 形态 Actor 蓝图未预先挂 ItemComponent 时，物品仅进入世界、不可收容。
	 * @return 形态 Actor；查表失败或生成失败返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "生成物品入世界")
	)
	AActor* SpawnItemInWorld(FName RowId, FTransform Transform);

	/**
	 * 从世界收容物品到容器。
	 * TakeItem 取回实例 → Destroy 形态 Actor → 提供目标容器则入容器，否则返回实例由调用方处置。
	 * @return 收容后的物品实例；入容器失败（满）或未提供容器时仍返回实例，无物品或入参非法返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点物库存|API",
		meta = (DisplayName = "收容物品出世界")
	)
	USingularisItem* CollectItem(
		USingularisItemComponent* ItemComponent,
		USingularisPocketComponent* TargetContainer = nullptr
	);

#pragma endregion
};
