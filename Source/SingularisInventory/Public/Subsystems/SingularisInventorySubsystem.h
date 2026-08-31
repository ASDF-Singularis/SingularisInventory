#pragma once

#include <CoreMinimal.h>
#include <Subsystems/GameInstanceSubsystem.h>

#include "SingularisInventorySubsystem.generated.h"

class USingularisItem;
class USingularisItemDefinition;
class AActor;

/**
 * 引力奇点库存子系统。
 *
 * 全局查询与编排服务：以 USingularisInventorySettings 配置的物品定义注册表为数据源，
 * 提供按物品实例 / 形态 Actor 类查询物品定义的易用 API，并承担物品入世界 / 收容的世界生命周期原语。
 * 蓝图经 GetGameInstanceSubsystem 节点可达。
 */
UCLASS(NotBlueprintable, BlueprintType)
class SINGULARISINVENTORY_API USingularisInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region Constructors

	USingularisInventorySubsystem();

#pragma endregion

#pragma region Subsystem Interface

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#pragma endregion

#pragma region API

	/** 物品实例背引用的定义；实例无效返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "获取物品定义")
	)
	USingularisItemDefinition* GetItemDefinition(USingularisItem* Item) const;

	/** 按形态 Actor 类查物品定义，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按形态Actor类获取物品定义")
	)
	USingularisItemDefinition* FindDefinitionByFormActorClass(TSubclassOf<AActor> FormActorClass) const;

	/**
	 * 生成物品入世界。
	 * 经物品实例背引用的定义查形态 Actor 类 → SpawnActor 形态 Actor → 绑定 ItemComponent → 开启物理。
	 * @return 形态 Actor；物品未配置形态 Actor 类、生成失败返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "生成物品入世界")
	)
	AActor* SpawnItemInWorld(USingularisItem* Item, const FTransform& Transform) const;

	/**
	 * 从世界收容物品：查找形态 Actor 上的 ItemComponent → TakeItem 取回实例 → Destroy 形态 Actor → 返回实例。
	 * 纯世界生命周期原语，不操纵容器；
	 * @return 收容后的物品实例；形态 Actor 无 ItemComponent 或无物品、入参非法返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "收容物品出世界")
	)
	USingularisItem* CollectItem(AActor* FormActor) const;

#pragma endregion
};
