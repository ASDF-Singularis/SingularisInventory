#include "Factories/SingularisItemActionFactory.h"

#include <Kismet2/KismetEditorUtilities.h>
#include <Objects/SingularisItemAction.h>

USingularisItemActionFactory::USingularisItemActionFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USingularisItemAction::StaticClass();
}

UObject* USingularisItemActionFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	const FName InName,
	const EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	// 1) 利用 KismetEditorUtilities 自动生成蓝图资产
	// 2) 强制将其基类指派为最新的物品动作基础类 USingularisItemAction
	return FKismetEditorUtilities::CreateBlueprint(
		USingularisItemAction::StaticClass(),
		InParent,
		InName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);
}

bool USingularisItemActionFactory::ShouldShowInNewMenu() const
{
	return true;
}

UClass* FAssetTypeActions_SingularisItemAction::GetSupportedClass() const
{
	return USingularisItemAction::StaticClass();
}
