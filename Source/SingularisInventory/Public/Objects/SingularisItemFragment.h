#pragma once

#include <CoreMinimal.h>
#include <GameplayTagAssetInterface.h>
#include <GameplayTagContainer.h>
#include <UObject/Object.h>

#include "SingularisItemFragment.generated.h"

struct FSingularisItemFragmentContext;

/**
 * 引力奇点物品片段。
 *
 * 片段是物品定义的组成单元：数据与逻辑内聚于一体（策略）。
 * 片段经 Trigger 消费自身数据并驱动世界变化，调用方只调 Trigger、不读片段字段。
 * 片段实现 IGameplayTagAssetInterface 自报响应标签，执行器按触发标签层级匹配后调用。
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class SINGULARISINVENTORY_API USingularisItemFragment : public UObject, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
#pragma region Parameter

	/**
	 * 响应标签：本片段响应的触发标签集合（支持层级匹配）。
	 * 作为 GetOwnedGameplayTags 默认实现的数据源，蓝图片段在此填数据即可。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品片段|参数",
		meta = (
			DisplayName = "响应标签",
			Categories = "Singularis.Inventory.Fragment",
			ForceSelection = "true"
		)
	)
	FGameplayTagContainer FragmentTags{};

#pragma endregion

#pragma region GameplayTags Interface

	/**
	 * 片段响应标签：返回本片段响应的触发标签集合（支持层级匹配）。
	 * 默认实现返回 FragmentTags 数据源；C++ 子类可覆写以动态计算自身响应范围。
	 * @param TagContainer 输出的响应标签集合
	 */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

#pragma endregion

#pragma region SPI

	/**
	 * 执行片段逻辑。
	 * @param Context 片段上下文
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品片段|SPI",
		meta = (DisplayName = "执行片段")
	)
	void Trigger(const FSingularisItemFragmentContext& Context);

#pragma endregion
};
