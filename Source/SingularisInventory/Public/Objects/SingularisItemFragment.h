#pragma once

#include <CoreMinimal.h>
#include <UObject/Object.h>

#include "SingularisItemFragment.generated.h"

struct FSingularisItemFragmentContext;

/**
 * 引力奇点物品片段。
 *
 * 片段是物品定义的组成单元：数据与逻辑内聚于一体（策略）。
 * 片段经 Trigger 消费自身数据并驱动世界变化，调用方只调 Trigger、不读片段字段。
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class SINGULARISINVENTORY_API USingularisItemFragment : public UObject
{
	GENERATED_BODY()

public:
#pragma region SPI

	/**
	 * 执行片段逻辑。
	 * @param Context 片段上下文
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品片段|",
		meta = (DisplayName = "执行片段")
	)
	void Trigger(const FSingularisItemFragmentContext& Context);

#pragma endregion
};
