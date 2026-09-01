#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Subsystems/GameInstanceSubsystem.h>

#include "SingularisInventorySubsystem.generated.h"

class USingularisItem;
class USingularisItemDefinition;
class AActor;

/**
 * 引力奇点库存子系统。
 *
 * 全局查询与编排服务：初始化时经物品形态注册表与物品定义资产构建内存双向映射
 * （ItemTag <-> FormActorClass），提供按物品实例 / 物品标签查询的易用 API，
 * 支持运行时动态注册 / 注销，并承担物品入世界 / 收容的世界生命周期原语。
 * 蓝图经 GetGameInstanceSubsystem 节点可达。
 */
UCLASS(NotBlueprintable, BlueprintType)
class SINGULARISINVENTORY_API USingularisInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:
#pragma region Internal Variable

	/** 物品标签 -> 形态 Actor 类（正向映射）。 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TSubclassOf<AActor>> TagToFormActorMap{};

	/** 形态 Actor 类 -> 物品标签（反向映射）。 */
	UPROPERTY(Transient)
	TMap<TSubclassOf<AActor>, FGameplayTag> FormActorToTagMap{};

	/** 物品标签 -> 物品定义（初始化经 AssetManager 扫描构建，强引用保持加载）。 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<USingularisItemDefinition>> TagToDefinitionMap{};

#pragma endregion

public:
#pragma region Constructors

	USingularisInventorySubsystem();

#pragma endregion

#pragma region Subsystem Interface

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#pragma endregion

#pragma region API

	/** 按物品标签查物品定义，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按物品标签获取物品定义")
	)
	USingularisItemDefinition* FindDefinitionByItemTag(const FGameplayTag& ItemTag) const;

	/** 按物品标签查形态 Actor 类，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按物品标签获取形态Actor类")
	)
	TSubclassOf<AActor> FindFormActorClass(const FGameplayTag& ItemTag) const;

	/** 按形态 Actor 类反查物品标签，未配置返回空标签。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按形态Actor类获取物品标签")
	)
	FGameplayTag FindItemTagByFormActorClass(const TSubclassOf<AActor> FormActorClass) const;

	/** 动态注册物品标签 -> 形态 Actor 类映射（替换旧关联，保证双向一致）。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "注册物品形态")
	)
	bool RegisterItemForm(const FGameplayTag& ItemTag, const TSubclassOf<AActor> FormActorClass);

	/** 动态注销物品标签 -> 形态 Actor 类映射。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "注销物品形态")
	)
	bool UnregisterItemForm(const FGameplayTag& ItemTag);

	/** 重建注册表：重新载入物品形态注册表与物品定义资产映射。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "重建注册表")
	)
	void RebuildRegistry();

	/**
	 * 生成物品入世界。
	 * 经物品实例背引用的定义取物品标签 → 查形态映射表取形态 Actor 类 → SpawnActor → 绑定 ItemComponent → 开启物理。
	 * @return 形态 Actor；物品无定义、未配置形态 Actor 类、生成失败返回 nullptr
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
