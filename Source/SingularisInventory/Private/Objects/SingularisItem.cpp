#include "Objects/SingularisItem.h"

#include <Net/UnrealNetwork.h>
#include <UObject/UObjectGlobals.h>

USingularisItem* USingularisItem::MaterializeFromDefinition(UObject* Outer, USingularisItemDefinition* Definition)
{
	// 1) 零信任校验：Outer 与定义必须有效
	if (!IsValid(Outer) || !IsValid(Definition))
		return nullptr;

	// 2) 创建运行时实例并背引用定义
	USingularisItem* const Materialized = NewObject<USingularisItem>(Outer);
	Materialized->SetDefinition(Definition);

	return Materialized;
}

void USingularisItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USingularisItem, Definition);
}

void USingularisItem::SetDefinition(USingularisItemDefinition* InDefinition)
{
	Definition = InDefinition;
}
