#pragma once

#include <CoreMinimal.h>
#include <GameplayTagAssetInterface.h>
#include <GameplayTagContainer.h>
#include <UObject/CoreNetTypes.h>
#include <UObject/Object.h>

#include "SingularisItemFragment.generated.h"

struct FSingularisItemFragmentContext;

/**
 * 引力奇点物品片段。
 *
 * 片段作为状态的载体，响应物品的触发标签，提供物品的状态数据。
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class SINGULARISINVENTORY_API USingularisItemFragment : public UObject, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
#pragma region Parameter

	/**
	 * 响应标签：本片段响应的触发标签集合（支持层级匹配）。
	 * 作为 GetOwnedGameplayTags 默认实现的数据源，蓝图片段在此填数据即可。
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点物品片段|参数",
		meta = (
			DisplayName = "响应标签",
			Categories = "Singularis.Inventory.Fragment",
			ForceSelection = "true",
			EditCondition = "bIsCDO", // 绑定布尔变量
			EditConditionHides // 当条件为 false 时，直接在面板隐藏
		)
	)
	FGameplayTagContainer FragmentTags{};

#pragma endregion

private:
#pragma region Internal Variable

	UPROPERTY(Transient, DuplicateTransient, NonTransactional)
	bool bIsCDO = false;

#pragma endregion

public:
#pragma region Object Interface

#if WITH_EDITOR

	virtual void PostInitProperties() override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;

#endif

	/** 支持网络复制：片段作为物品实例的复制子对象同步到客户端。 */
	virtual bool IsSupportedForNetworking() const override { return true; }

	/** 声明需要复制的属性（基类无复制属性，供子类覆写扩展）。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion

#pragma region GameplayTags Interface

	/**
	 * 片段响应标签：返回本片段响应的触发标签集合（支持层级匹配）。
	 * 默认实现返回 FragmentTags 数据源；C++ 子类可覆写以动态计算自身响应范围。
	 * @param TagContainer 输出的响应标签集合
	 */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

#pragma endregion
};
