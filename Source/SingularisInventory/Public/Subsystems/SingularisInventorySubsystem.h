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
 * 全局查询与编排服务：初始化时经物品形态注册表与物品定义资产构建内存映射
 * （物品标签 -> 物品形态 / 物品标签 -> 物品定义，并推导物品定义 <-> 物品形态双向映射），
 * 提供按物品实例 / 物品标签 / 物品定义查询的易用 API，
 * 支持运行时动态注册 / 注销，并承担物品入世界 / 收容的世界生命周期原语。
 * 蓝图经 GetGameInstanceSubsystem 节点可达。
 */
UCLASS(NotBlueprintable, BlueprintType)
class SINGULARISINVENTORY_API USingularisInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region Internal Variable

	/** 物品标签 -> 物品形态。 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TSubclassOf<AActor>> TagToFormActorMap{};

	/** 物品标签 -> 物品定义（初始化经 AssetManager 扫描构建，强引用保持加载）。 */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<USingularisItemDefinition>> TagToDefinitionMap{};

	/** 物品定义 -> 物品形态（由标签映射推导，随标签映射保持一致）。 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<USingularisItemDefinition>, TSubclassOf<AActor>> DefinitionToFormActorMap{};

	/** 物品形态 -> 物品定义（由标签映射推导，随标签映射保持一致）。 */
	UPROPERTY(Transient)
	TMap<TSubclassOf<AActor>, TObjectPtr<USingularisItemDefinition>> FormActorToDefinitionMap{};

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
		meta = (DisplayName = "按 Tag 获取物品定义")
	)
	USingularisItemDefinition* FindDefinitionByItemTag(const FGameplayTag& ItemTag) const;

	/** 按物品标签查物品形态，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按 Tag 获取物品形态")
	)
	TSubclassOf<AActor> FindFormActorClass(const FGameplayTag& ItemTag) const;

	/** 按物品定义查物品形态，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按物品定义获取物品形态")
	)
	TSubclassOf<AActor> FindFormActorClassByDefinition(USingularisItemDefinition* Definition) const;

	/** 按物品形态反查物品定义，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "按物品形态获取物品定义")
	)
	USingularisItemDefinition* FindDefinitionByFormActorClass(const TSubclassOf<AActor> FormActorClass) const;

	/** 动态注册物品定义 -> 物品形态映射（替换旧关联，保证映射一致）。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "注册物品形态")
	)
	bool RegisterItemForm(USingularisItemDefinition* Definition, const TSubclassOf<AActor> FormActorClass);

	/** 动态注销物品定义 -> 物品形态映射。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "注销物品形态")
	)
	bool UnregisterItemForm(USingularisItemDefinition* Definition);

	/** 重建注册表：重新载入物品形态注册表与物品定义资产映射。 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "重建注册表")
	)
	void RebuildRegistry();

	/**
	 * 生成物品入世界。
	 * 经物品实例背引用的定义查形态映射表取物品形态 → SpawnActor → 绑定 ItemComponent → 开启物理。
	 * @return 物品形态；物品无定义、未配置物品形态、生成失败返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "生成物品入世界")
	)
	AActor* SpawnItemInWorld(USingularisItem* Item, const FTransform& Transform) const;

	/**
	 * 从世界收容物品：查找物品形态上的 ItemComponent → TakeItem 取回实例 → Destroy 物品形态 → 返回实例。
	 * 纯世界生命周期原语，不操纵容器；
	 * @return 收容后的物品实例；物品形态无 ItemComponent 或无物品、入参非法返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "SingularisInventory|引力奇点库存|API",
		meta = (DisplayName = "收容物品出世界")
	)
	USingularisItem* CollectItem(AActor* FormActor) const;

#pragma endregion

private:
#pragma region Internal Function

	/** 经标签映射桥接重建物品定义 <-> 物品形态双向映射。 */
	void RebuildDefinitionFormMaps();

#pragma endregion
};
