#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <UObject/Object.h>

#include "SingularisItemFragment.generated.h"

struct FSingularisItemFragmentContext;

/**
 * 引力奇点物品片段。
 *
 * 片段是物品定义的组成单元：数据与逻辑内聚于一体（策略）。
 * 片段经 Trigger 消费自身数据并驱动世界变化，调用方只调 Trigger、不读片段字段。
 * 片段经 Tags 接口自报响应标签，执行器按触发标签层级匹配后调用。
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class SINGULARISINVENTORY_API USingularisItemFragment : public UObject
{
	GENERATED_BODY()

public:
#pragma region SPI

	/**
	 * 片段响应标签：自报本片段响应哪些触发标签（支持层级匹配）。
	 * 默认实现返回空容器，子类覆写以声明自身响应范围。
	 * @param OutTags 输出的响应标签集合
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品片段|SPI",
		meta = (DisplayName = "获取片段标签")
	)
	void GetFragmentTags(FGameplayTagContainer& OutTags) const;

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
