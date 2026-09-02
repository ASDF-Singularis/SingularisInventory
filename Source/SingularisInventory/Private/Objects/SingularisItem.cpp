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

USingularisItemFragment* USingularisItem::FindFragmentByClass(
	const TSubclassOf<USingularisItemFragment> FragmentClass
) const
{
	// 1) 卫语句：定义与片段类必须有效
	if (!IsValid(Definition) || !IsValid(FragmentClass))
		return nullptr;

	// 2) 线性扫描首个类型匹配片段
	for (const TObjectPtr<USingularisItemFragment>& Fragment : Definition->Fragments)
	{
		if (IsValid(Fragment) && Fragment->IsA(FragmentClass.Get()))
			return Fragment;
	}

	return nullptr;
}

USingularisItemFragment* USingularisItem::FindFragmentByTag(const FGameplayTag& Tag) const
{
	// 复用全量查询取首元素，标签匹配逻辑保持单点
	const TArray<USingularisItemFragment*> Matches = FindFragmentsByTag(Tag);
	return Matches.IsEmpty() ? nullptr : Matches[0];
}

TArray<USingularisItemFragment*> USingularisItem::FindFragmentsByTag(const FGameplayTag& Tag) const
{
	// 1) 卫语句：定义与标签必须有效
	if (!IsValid(Definition) || !Tag.IsValid())
		return {};

	TArray<USingularisItemFragment*> Matches;

	// 2) 线性扫描全部响应标签匹配片段（层级匹配）
	for (const TObjectPtr<USingularisItemFragment>& Fragment : Definition->Fragments)
	{
		if (!IsValid(Fragment))
			continue;

		FGameplayTagContainer OwnedTags;
		Fragment->GetOwnedGameplayTags(OwnedTags);
		if (OwnedTags.HasTag(Tag))
			Matches.Add(Fragment);
	}

	return Matches;
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
