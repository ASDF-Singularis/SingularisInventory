#include "SingularisInventoryEditor.h"

#include <AssetToolsModule.h>

#include "Factories/SingularisItemFactory.h"
#include "Factories/SingularisItemFormActorFactory.h"
#include "Factories/SingularisPocketWidgetFactory.h"

#define LOCTEXT_NAMESPACE "FSingularisInventoryEditorModule"

void FSingularisInventoryEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const EAssetTypeCategories::Type SingularisPluginCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName("Singularis"),
		LOCTEXT("SingularisCategory", "Singularis")
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
		MakeShareable(new FAssetTypeActions_SingularisPocketWidget(SingularisPluginCategory))
	);
}

void FSingularisInventoryEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

		for (const auto& Action : CreatedAssetTypeActions)
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
	}

	CreatedAssetTypeActions.Empty();
}

void FSingularisInventoryEditorModule::RegisterAssetTypeAction(
	IAssetTools& AssetTools,
	const TSharedRef<IAssetTypeActions>& Action
)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSingularisInventoryEditorModule, SingularisInventoryEditor)
