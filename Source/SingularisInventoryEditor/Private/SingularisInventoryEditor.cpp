#include "SingularisInventoryEditor.h"

#include <AssetToolsModule.h>

#include "Factories/SingularisItemDefinitionFactory.h"
#include "Factories/SingularisItemFactory.h"
#include "Factories/SingularisItemFormActorFactory.h"
#include "Factories/SingularisItemFragmentFactory.h"
#include "Factories/SingularisPocketWidgetFactory.h"

DEFINE_LOG_CATEGORY(LogSingularisInventoryEditor);

#define LOCTEXT_NAMESPACE "FSingularisInventoryEditorModule"

void FSingularisInventoryEditorModule::StartupModule()
{
	// 1) 登记 Singularis 资产分类
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const EAssetTypeCategories::Type SingularisPluginCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName("Singularis"),
		LOCTEXT("SingularisCategory", "Singularis")
	);

	// 2) 登记五类资产行为
	RegisterAssetTypeAction(
		AssetTools,
		MakeShareable(new FAssetTypeActions_SingularisItemDefinition(SingularisPluginCategory))
	);

	RegisterAssetTypeAction(
		AssetTools,
		MakeShareable(new FAssetTypeActions_SingularisItem(SingularisPluginCategory))
	);

	RegisterAssetTypeAction(
		AssetTools,
		MakeShareable(new FAssetTypeActions_SingularisItemFormActor(SingularisPluginCategory))
	);

	RegisterAssetTypeAction(
		AssetTools,
		MakeShareable(new FAssetTypeActions_SingularisItemFragment(SingularisPluginCategory))
	);

	RegisterAssetTypeAction(
		AssetTools,
		MakeShareable(new FAssetTypeActions_SingularisPocketWidget(SingularisPluginCategory))
	);

	UE_LOG(
		LogSingularisInventoryEditor,
		Display,
		TEXT("StartupModule：编辑器模块初始化完成，已登记 %d 项资产类型行为"),
		CreatedAssetTypeActions.Num()
	);
}

void FSingularisInventoryEditorModule::ShutdownModule()
{
	// 1) 编辑器关闭时 AssetTools 可能已卸载，需先检查再反注册
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

		for (const auto& Action : CreatedAssetTypeActions)
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
	}

	UE_LOG(
		LogSingularisInventoryEditor,
		Display,
		TEXT("ShutdownModule：编辑器模块卸载，已反注册 %d 项资产类型行为"),
		CreatedAssetTypeActions.Num()
	);

	CreatedAssetTypeActions.Empty();
}

void FSingularisInventoryEditorModule::RegisterAssetTypeAction(
	IAssetTools& AssetTools,
	const TSharedRef<IAssetTypeActions>& Action
)
{
	// 1) 登记资产行为并留存引用，供卸载时反注册
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSingularisInventoryEditorModule, SingularisInventoryEditor)
