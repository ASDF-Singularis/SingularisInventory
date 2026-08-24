#pragma once

#include <CoreMinimal.h>
#include <Subsystems/GameInstanceSubsystem.h>

#include "DataTables/SingularisItemRow.h"
#include "SingularisInventoryItemSubsystem.generated.h"

class UDataTable;
class USingularisItem;
class UTexture2D;
class AActor;

/**
 * 引力奇点物品查询子系统。
 *
 * 全局查询服务：以 USingularisInventorySettings 配置的物品表为数据源，
 * 提供按物品实例 / 类查询静态数据与形态 Actor 类的易用 API。
 * 蓝图经 GetGameInstanceSubsystem 节点可达。
 */
UCLASS(NotBlueprintable, BlueprintType)
class SINGULARISINVENTORY_API USingularisInventoryItemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region Constructors

	USingularisInventoryItemSubsystem();

#pragma endregion

#pragma region Subsystem Interface

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#pragma endregion

#pragma region API

	/** 全局物品数据表。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "获取物品数据表")
	)
	UDataTable* GetItemTable() const;

	/** 按物品实例查行指针，未找到返回 nullptr（C++ 核心）。 */
	const FSingularisItemRow* FindItemRow(const USingularisItem* Item) const;

	/** 按物品类查行指针，未找到返回 nullptr（C++ 核心）。 */
	const FSingularisItemRow* FindItemRowByClass(TSubclassOf<USingularisItem> ItemClass) const;

	/** 按形态 Actor 类查行指针，未找到返回 nullptr（C++ 核心）。 */
	const FSingularisItemRow* FindItemRowByFormActorClass(TSubclassOf<AActor> FormActorClass) const;

	/** 按物品实例查整行；找到返回 true 并输出行数据。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "获取物品数据行")
	)
	bool TryGetItemRow(USingularisItem* Item, FSingularisItemRow& OutRow) const;

	/** 按物品类查整行。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "按类获取物品数据行")
	)
	bool TryGetItemRowByClass(TSubclassOf<USingularisItem> ItemClass, FSingularisItemRow& OutRow) const;

	/** 按形态 Actor 类查整行。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "按形态Actor类获取物品数据行")
	)
	bool TryGetItemRowByFormActorClass(TSubclassOf<AActor> FormActorClass, FSingularisItemRow& OutRow) const;

	/** 按物品实例查形态 Actor 类，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "获取物品形态Actor类")
	)
	TSubclassOf<AActor> GetFormActorClass(USingularisItem* Item) const;

	/** 按物品类查形态 Actor 类。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "按类获取物品形态Actor类")
	)
	TSubclassOf<AActor> GetFormActorClassByClass(TSubclassOf<USingularisItem> ItemClass) const;

	/** 按物品实例查图标，未配置返回 nullptr。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "获取物品图标")
	)
	UTexture2D* GetItemIcon(USingularisItem* Item) const;

	/** 按物品类查图标。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "按类获取物品图标")
	)
	UTexture2D* GetItemIconByClass(TSubclassOf<USingularisItem> ItemClass) const;

	/** 按物品实例查名称，未配置返回空文本。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "获取物品名称")
	)
	FText GetItemName(USingularisItem* Item) const;

	/** 按物品类查名称。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "按类获取物品名称")
	)
	FText GetItemNameByClass(TSubclassOf<USingularisItem> ItemClass) const;

	/** 按物品实例查描述，未配置返回空文本。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "获取物品描述")
	)
	FText GetItemDescription(USingularisItem* Item) const;

	/** 按物品类查描述。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物库存|查询",
		meta = (DisplayName = "按类获取物品描述")
	)
	FText GetItemDescriptionByClass(TSubclassOf<USingularisItem> ItemClass) const;

#pragma endregion
};
