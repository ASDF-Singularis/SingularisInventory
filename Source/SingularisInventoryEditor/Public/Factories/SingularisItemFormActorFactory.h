#pragma once

#include <CoreMinimal.h>
#include <AssetTypeActions/AssetTypeActions_Blueprint.h>
#include <Factories/Factory.h>

#include "SingularisItemFormActorFactory.generated.h"

/**
 * 物品形态 Actor 工厂类
 */
UCLASS()
class SINGULARISINVENTORYEDITOR_API USingularisItemFormActorFactory : public UFactory
{
	GENERATED_BODY()

public:
	USingularisItemFormActorFactory();

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn
	) override;

	virtual bool ShouldShowInNewMenu() const override;
};

/**
 * 物品形态 Actor 资产类型操作 (定义编辑器右键菜单行为)
 */
class FAssetTypeActions_SingularisItemFormActor : public FAssetTypeActions_Blueprint
{
public:
	explicit FAssetTypeActions_SingularisItemFormActor(const EAssetTypeCategories::Type InAssetCategory)
		: AssetTypeCategory(InAssetCategory) {}

	virtual FText GetName() const override
	{
		return NSLOCTEXT(
			"SingularisInventoryEditor",
			"AssetTypeActions_SingularisItemFormActor",
			"Singularis Item Form Actor"
		);
	}

	virtual FColor GetTypeColor() const override { return FColor(155, 89, 182); }

	virtual UClass* GetSupportedClass() const override;

	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override
	{
		// 1) 动态实例化工厂对象以接管该资产蓝图的创建流程
		USingularisItemFormActorFactory* Factory = NewObject<USingularisItemFormActorFactory>();
		return Factory;
	}

	virtual uint32 GetCategories() override { return AssetTypeCategory; }

	virtual const TArray<FText>& GetSubMenus() const override
	{
		// 1) 将资产收纳至右键菜单的指定子目录中
		static const TArray SubMenus = {
			FText::FromString("SingularisInventory"),
		};

		return SubMenus;
	}

private:
	EAssetTypeCategories::Type AssetTypeCategory;
};
