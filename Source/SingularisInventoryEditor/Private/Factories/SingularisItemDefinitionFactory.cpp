#include "Factories/SingularisItemDefinitionFactory.h"

#include <DataAssets/SingularisItemDefinition.h>

USingularisItemDefinitionFactory::USingularisItemDefinitionFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = USingularisItemDefinition::StaticClass();
}

UObject* USingularisItemDefinitionFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	const FName InName,
	const EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	// 1) 数据资产直接在包内 NewObject，无需经蓝图生成
	return NewObject<USingularisItemDefinition>(InParent, InClass, InName, Flags);
}

bool USingularisItemDefinitionFactory::ShouldShowInNewMenu() const
{
	return true;
}

UClass* FAssetTypeActions_SingularisItemDefinition::GetSupportedClass() const
{
	return USingularisItemDefinition::StaticClass();
}
