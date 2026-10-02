#pragma once

#include <CoreMinimal.h>
#include <UObject/Interface.h>

#include "SingularisItemFormActorInterface.generated.h"

class USingularisItemComponent;

UINTERFACE(Blueprintable, BlueprintType)
class USingularisItemFormActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 引力奇点物品形态接口。
 *
 * 物品形态的自描述契约：暴露其挂接的 USingularisItemComponent，供库存子系统与
 * 项目侧以统一方式获取物品组件，无需强类型依赖 ASingularisItemFormActor。
 */
class SINGULARISINVENTORY_API ISingularisItemFormActorInterface
{
	GENERATED_BODY()

public:
	/**
	 * 获取物品形态挂接的物品组件。
	 * @return 物品组件；未挂接返回 nullptr
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "引力奇点物品演员形态接口",
		meta = (DisplayName = "获取物品组件")
	)
	USingularisItemComponent* GetItemComponent();
};
