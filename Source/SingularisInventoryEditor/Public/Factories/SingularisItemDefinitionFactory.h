#pragma once

#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>
#include <Factories/Factory.h>

#include "SingularisItemDefinitionFactory.generated.h"

/**
 * 物品定义工厂类
 */
UCLASS()
class SINGULARISINVENTORYEDITOR_API USingularisItemDefinitionFactory : public UFactory
{
	GENERATED_BODY()

public:
	USingularisItemDefinitionFactory();

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
 * 物品定义资产类型操作 (定义编辑器右键菜单行为)
 */
class FAssetTypeActions_SingularisItemDefinition : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_SingularisItemDefinition(const EAssetTypeCategories::Type InAssetCategory)
		: AssetTypeCategory(InAssetCategory) {}

	virtual FText GetName() const override
	{
		return NSLOCTEXT(
			"SingularisInventoryEditor",
			"AssetTypeActions_SingularisItemDefinition",
			"Singularis Item Definition"
		);
	}

	virtual FColor GetTypeColor() const override { return FColor(255, 168, 46); }

	virtual UClass* GetSupportedClass() const override;

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
