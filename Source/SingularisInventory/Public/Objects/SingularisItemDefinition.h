#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Engine/DataAsset.h>
#include <UObject/PrimaryAssetId.h>

#include "Types/SingularisItemType.h"
#include "SingularisItemDefinition.generated.h"

class UTexture2D;
class AActor;

/**
 * 引力奇点物品定义。
 *
 * 单一数据源（SSOT）：每种物品一个定义资产，聚合物品的静态元数据、
 * 形态 Actor 映射与片段管线映射（FSingularisItemFragmentPipeline）。
 * 运行时实例 USingularisItem 由本定义物化而来，背引用本定义查询配置。
 */
UCLASS(BlueprintType)
class SINGULARISINVENTORY_API USingularisItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 主资产类型标识，AssetManager 按此类型发现物品定义。 */
	static const FPrimaryAssetType ItemType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#pragma region Parameter

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

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (DisplayName = "形态Actor类")
	)
	TSubclassOf<AActor> FormActorClass = nullptr;

	/**
	 * 片段管线映射：按片段标签匹配到有序片段管线。
	 * 键为片段标签（支持层级匹配），值为有序执行的片段管线。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品定义|参数",
		meta = (
			DisplayName = "片段映射",
			Categories = "Singularis.Inventory.Fragment",
			ForceSelection = "true"
		)
	)
	TMap<FGameplayTag, FSingularisItemFragmentPipeline> FragmentMapping{};

#pragma endregion
};
