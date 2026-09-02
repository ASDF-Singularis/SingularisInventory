#pragma once

#include <CoreMinimal.h>
#include <type_traits>
#include <UObject/CoreNetTypes.h>
#include <UObject/Object.h>

#include "DataAssets/SingularisItemDefinition.h"
#include "Objects/SingularisItemFragment.h"
#include "SingularisItem.generated.h"

/**
 * 引力奇点物品（运行时实例）。
 *
 * 每件进入世界 / 容器的物品都是一个运行时实例：背引用其物品定义
 * （USingularisItemDefinition）查询静态配置与平铺片段数组。
 * 实例经 USingularisItemComponent / USingularisPocketComponent 注册为网络复制子对象。
 *
 * 默认物化为本基类；如需扩展运行时状态，可在项目设置配置一个全局子类作为物品实例类。
 */
UCLASS(Abstract, BlueprintType)
class SINGULARISINVENTORY_API USingularisItem : public UObject
{
	GENERATED_BODY()

#pragma region Internal Variable

	/** 物品定义资产，复制到客户端使远端可查询配置。 */
	UPROPERTY(Replicated)
	TObjectPtr<USingularisItemDefinition> Definition = nullptr;

#pragma endregion

public:
#pragma region Constructors

	USingularisItem();

#pragma endregion

#pragma region UObject Interface

	/** 声明需要复制的属性。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion

#pragma region State

	/** 物品定义（单一数据源）。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物品|State",
		meta = (DisplayName = "获取物品定义")
	)
	USingularisItemDefinition* GetDefinition() const { return Definition; }
	
#pragma endregion 
	
#pragma region API

	/** 按片段类查询首个匹配片段。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物品|API",
		meta = (DisplayName = "按类查询片段")
	)
	USingularisItemFragment* FindFragmentByClass(TSubclassOf<USingularisItemFragment> FragmentClass) const;

	/** 按响应标签查询首个匹配片段（层级匹配）。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物品|API",
		meta = (DisplayName = "按标签查询片段")
	)
	USingularisItemFragment* FindFragmentByTag(const FGameplayTag& Tag) const;

	/** 按响应标签查询全部匹配片段（层级匹配），供执行器逐片段执行。 */
	UFUNCTION(
		BlueprintPure,
		Category = "SingularisInventory|引力奇点物品|API",
		meta = (DisplayName = "按标签查询全部片段")
	)
	TArray<USingularisItemFragment*> FindFragmentsByTag(const FGameplayTag& Tag) const;

	/**
	 * 按片段类型查询首个匹配片段（C++ 便捷模板）。
	 * @return 首个类型匹配的片段指针；无匹配返回 nullptr
	 */
	template <typename TFragment>
	TFragment* FindFragment() const
	{
		static_assert(
			std::is_base_of_v<USingularisItemFragment, TFragment>,
			"TFragment must derive from USingularisItemFragment"
		);
		return Cast<TFragment>(FindFragmentByClass(TFragment::StaticClass()));
	}

#pragma endregion

#pragma region SPI

	/**
	 * 从物品定义物化出一个独立的运行时实例。
	 *
	 * 物化实例的 Outer 设为调用方传入的 Outer（推荐 UWorld，使生命周期脱离形态 Actor / 组件），
	 * 背引用定义查询静态配置与平铺片段数组。仅用于权威端 BeginPlay 阶段。
	 * @param Outer 物化实例的外层；生命周期归属于此对象
	 * @param ItemDefinition 物品定义资产
	 * @return 物化出的运行时实例；Outer 或定义无效返回 nullptr
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点物品|SPI",
		meta = (DisplayName = "从定义物化实例")
	)
	static USingularisItem* MaterializeFromDefinition(UObject* Outer, USingularisItemDefinition* ItemDefinition);

#pragma endregion

private:
#pragma region Internal Function

	/** 建立背引用定义。 */
	void SetDefinition(USingularisItemDefinition* InDefinition);

#pragma endregion
};
