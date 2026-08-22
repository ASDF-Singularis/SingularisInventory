#pragma once

#include <CoreMinimal.h>

#include "SingularisItem.generated.h"

class UObject;
class UWorld;

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, CollapseCategories)
class SINGULARISINVENTORY_API USingularisItem : public UObject
{
	GENERATED_BODY()

public:
#pragma region SPI

	/**
	 * 从设计期模板实例物化出一个独立的运行时实例。
	 *
	 * 物化实例的 Outer 设为调用方传入的 Outer（推荐 UWorld，使生命周期脱离 FormActor / Component），
	 * 属性从模板复制而来，与模板无引用关系。仅用于权威端 BeginPlay 阶段，
	 * 将编辑器配置的 Instanced 模板转化为可被 BindItem / AddItem 等运行时 API 接管的实例。
	 * @param Outer 物化实例的外层；生命周期归属于此对象
	 * @param Template 设计期配置的模板实例
	 * @return 物化出的独立运行时实例；Outer 或模板无效返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品|SPI",
		meta = (DisplayName = "从模板物化实例")
	)
	static USingularisItem* MaterializeFromTemplate(UObject* Outer, const USingularisItem* Template);

#pragma endregion
};
