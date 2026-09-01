#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Engine/DataAsset.h>
#include <UObject/PrimaryAssetId.h>

#include "SingularisItemDefinition.generated.h"

class UTexture2D;
class USingularisItemFragment;

/**
 * 引力奇点物品定义。
 *
 * 单一数据源（SSOT）：每种物品一个定义资产，聚合物品的静态元数据、
 * 形态 Actor 映射与平铺片段数组（USingularisItemFragment）。
 * 运行时实例 USingularisItem 由本定义物化而来，背引用本定义查询配置。
 */
UCLASS(BlueprintType)
class SINGULARISINVENTORY_API USingularisItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 主资产类型标识，AssetManager 按此类型发现物品定义。 */
	static const FPrimaryAssetType ItemType;

	/** 主资产 ID：类型 + 资产名。 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#pragma region Parameter

	/**
	 * 物品标签：物品的唯一标识，作为与形态 Actor 映射的桥梁。
	 * 形态 Actor 类经物品形态注册表（ItemFormTable）按此标签查得。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (
			DisplayName = "物品标签",
			Categories = "Singularis.Inventory.Item",
			ForceSelection = "true"
		)
	)
	FGameplayTag ItemTag{};

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (DisplayName = "名称")
	)
	FText Name{};

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (DisplayName = "描述")
	)
	FText Description{};

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (DisplayName = "图标")
	)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/**
	 * 物品片段：平铺组合的片段数组，数组顺序即执行顺序。
	 * 每个片段经 Tags 接口自报响应标签，执行器按触发标签层级匹配后逐片段执行。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (DisplayName = "物品片段")
	)
	TArray<TObjectPtr<USingularisItemFragment>> Fragments{};

#pragma endregion
};
