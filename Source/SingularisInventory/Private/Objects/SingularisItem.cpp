#include "Objects/SingularisItem.h"

#include <Net/UnrealNetwork.h>
#include <UObject/UObjectGlobals.h>

#include "Configs/SingularisInventorySettings.h"

USingularisItem::USingularisItem() {}

void USingularisItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USingularisItem, Definition);
}

USingularisItem* USingularisItem::MaterializeFromDefinition(UObject* Outer, USingularisItemDefinition* ItemDefinition)
{
	// 1) 零信任校验：Outer 与定义必须有效
	if (!IsValid(Outer) || !IsValid(ItemDefinition))
		return nullptr;

	// 2) 解析实例类：Settings 配置优先，未配置回退 USingularisItem 基类
	const UClass* InstanceClass = StaticClass();
	if (const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>())
	{
		if (IsValid(Settings->ItemClass))
			InstanceClass = Settings->ItemClass.Get();
	}

	// 3) 按实例类创建运行时实例并背引用定义
	USingularisItem* const Materialized = NewObject<USingularisItem>(Outer, InstanceClass);
	Materialized->SetDefinition(ItemDefinition);

	return Materialized;
}

void USingularisItem::SetDefinition(USingularisItemDefinition* InDefinition)
{
	Definition = InDefinition;
}
