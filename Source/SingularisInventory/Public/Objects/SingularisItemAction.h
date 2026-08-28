#pragma once

#include <CoreMinimal.h>
#include <UObject/Object.h>

#include "SingularisItemAction.generated.h"

struct FSingularisItemActionContext;

/**
 * 引力奇点物品动作
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories)
class SINGULARISINVENTORY_API USingularisItemAction : public UObject
{
	GENERATED_BODY()

public:
#pragma region SPI

	/**
	 * 执行物品动作。
	 * @param Context 动作上下文
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品动作|",
		meta = (DisplayName = "执行动作")
	)
	void Execute(const FSingularisItemActionContext& Context);

#pragma endregion
};
