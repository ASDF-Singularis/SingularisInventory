#include "Factories/SingularisItemFragmentFactory.h"

#include <Kismet2/KismetEditorUtilities.h>
#include <Objects/SingularisItemFragment.h>

USingularisItemFragmentFactory::USingularisItemFragmentFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USingularisItemFragment::StaticClass();
}

UObject* USingularisItemFragmentFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	const FName InName,
	const EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	// 1) 利用 KismetEditorUtilities 自动生成蓝图资产
	// 2) 强制将其基类指派为最新的物品片段基础类 USingularisItemFragment
	return FKismetEditorUtilities::CreateBlueprint(
			USingularisItemFragment::StaticClass(),
			InParent,
			InName,
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass(),
			NAME_None
	);
}

bool USingularisItemFragmentFactory::ShouldShowInNewMenu() const
{
	return true;
}

UClass* FAssetTypeActions_SingularisItemFragment::GetSupportedClass() const
{
	return USingularisItemFragment::StaticClass();
}
