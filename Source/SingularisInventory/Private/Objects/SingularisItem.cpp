#include "Objects/SingularisItem.h"

#include <Engine/Engine.h>
#include <UObject/UObjectGlobals.h>

USingularisItem* USingularisItem::MaterializeFromTemplate(UObject* Outer, const USingularisItem* Template)
{
	// 1) 零信任校验：Outer 与模板必须有效
	if (!IsValid(Outer) || !IsValid(Template))
		return nullptr;

	// 2) 按模板类在新 Outer 下创建独立实例，复制模板属性使其脱离模板引用关系
	USingularisItem* const Materialized = NewObject<USingularisItem>(Outer, Template->GetClass());
	UEngine::CopyPropertiesForUnrelatedObjects(const_cast<USingularisItem*>(Template), Materialized);

	return Materialized;
}
