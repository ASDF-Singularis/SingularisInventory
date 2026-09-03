#include "Objects/SingularisItem.h"

#include <Net/UnrealNetwork.h>
#include <UObject/UObjectGlobals.h>

#include "Configs/SingularisInventorySettings.h"

USingularisItem::USingularisItem() {}

void USingularisItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USingularisItem, Definition);
	DOREPLIFETIME(USingularisItem, Fragments);
}

USingularisItemFragment* USingularisItem::FindFragmentByClass(
	const TSubclassOf<USingularisItemFragment> FragmentClass
) const
{
	// 复用全量查询取首元素，类匹配逻辑保持单点
	const TArray<USingularisItemFragment*> Matches = FindFragmentsByClass(FragmentClass);
	return Matches.IsEmpty() ? nullptr : Matches[0];
}

bool USingularisItem::HasFragmentByClass(const TSubclassOf<USingularisItemFragment> FragmentClass) const
{
	// 复用类查询，空指针即不存在
	return FindFragmentByClass(FragmentClass) != nullptr;
}

TArray<USingularisItemFragment*> USingularisItem::FindFragmentsByClass(
	const TSubclassOf<USingularisItemFragment> FragmentClass
) const
{
	// 1) 卫语句：片段类必须有效
	if (!IsValid(FragmentClass))
		return {};

	TArray<USingularisItemFragment*> Matches;

	// 2) 线性扫描全部类型匹配片段（含派生类）
	for (const TObjectPtr<USingularisItemFragment>& Fragment : Fragments)
	{
		if (IsValid(Fragment) && Fragment->IsA(FragmentClass.Get()))
			Matches.Add(Fragment);
	}

	return Matches;
}

USingularisItemFragment* USingularisItem::FindFragmentByTag(const FGameplayTag& Tag) const
{
	// 复用全量查询取首元素，标签匹配逻辑保持单点
	const TArray<USingularisItemFragment*> Matches = FindFragmentsByTag(Tag);
	return Matches.IsEmpty() ? nullptr : Matches[0];
}

bool USingularisItem::HasFragmentByTag(const FGameplayTag& Tag) const
{
	// 复用标签查询，空指针即不存在
	return FindFragmentByTag(Tag) != nullptr;
}

TArray<USingularisItemFragment*> USingularisItem::FindFragmentsByTag(const FGameplayTag& Tag) const
{
	// 1) 卫语句：标签必须有效
	if (!Tag.IsValid())
		return {};

	TArray<USingularisItemFragment*> Matches;

	// 2) 线性扫描全部响应标签匹配片段（层级匹配）
	for (const TObjectPtr<USingularisItemFragment>& Fragment : Fragments)
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
	Materialized->InstantiateFragments();

	return Materialized;
}

void USingularisItem::SetDefinition(USingularisItemDefinition* InDefinition)
{
	Definition = InDefinition;
}

void USingularisItem::InstantiateFragments()
{
	// 1) 卫语句：定义无效直接返回
	if (!IsValid(Definition))
		return;

	Fragments.Reset();

	// 2) 逐模板克隆为独立运行时副本，拷贝模板配置值作为初始状态
	for (const TObjectPtr<USingularisItemFragment>& Template : Definition->Fragments)
	{
		if (!IsValid(Template))
			continue;

		USingularisItemFragment* const Fragment = NewObject<USingularisItemFragment>(
			this,
			Template->GetClass(),
			NAME_None,
			RF_NoFlags,
			Template.Get()
		);
		Fragments.Add(Fragment);
	}
}
