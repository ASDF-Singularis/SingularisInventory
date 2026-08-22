#pragma once

#include <CoreMinimal.h>
#include <Factories/Factory.h>

#include "AssetTypeActions/AssetTypeActions_Blueprint.h"
#include "Widgets/SingularisPocketWidget.h"
#include "SingularisPocketWidgetFactory.generated.h"

/**
 * 口袋控件工厂类
 */
UCLASS()
class SINGULARISINVENTORYEDITOR_API USingularisPocketWidgetFactory : public UFactory
{
	GENERATED_BODY()

public:
	USingularisPocketWidgetFactory();
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
 * 口袋控件资产类型操作 (定义编辑器右键菜单行为)
 */
class FAssetTypeActions_SingularisPocketWidget : public FAssetTypeActions_Blueprint
{
public:
	explicit FAssetTypeActions_SingularisPocketWidget(const EAssetTypeCategories::Type InAssetCategory)
		: AssetTypeCategory(InAssetCategory) {}

	virtual FText GetName() const override
	{
		return NSLOCTEXT(
			"SingularisInventoryEditor",
			"AssetTypeActions_SingularisPocketWidget",
			"Singularis Pocket Widget"
		);
	}

	virtual FColor GetTypeColor() const override { return FColor(46, 160, 96); }

	virtual UClass* GetSupportedClass() const override { return USingularisPocketWidget::StaticClass(); }

	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override
	{
		// 这里创建一个工厂实例给编辑器使用
		USingularisPocketWidgetFactory* Factory = NewObject<USingularisPocketWidgetFactory>();
		return Factory;
	}

	virtual uint32 GetCategories() override { return AssetTypeCategory; }

	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray SubMenus = {
			FText::FromString("SingularisInventory"),
		};

		return SubMenus;
	}

private:
	EAssetTypeCategories::Type AssetTypeCategory;
};
