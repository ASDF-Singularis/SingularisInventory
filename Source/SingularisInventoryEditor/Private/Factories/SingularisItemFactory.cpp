#include "Factories/SingularisItemFactory.h"

#include <Kismet2/KismetEditorUtilities.h>
#include <Objects/SingularisItem.h>

USingularisItemFactory::USingularisItemFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USingularisItem::StaticClass();
}

UObject* USingularisItemFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	const FName InName,
	const EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	// 1) 利用 KismetEditorUtilities 自动生成蓝图资产
	// 2) 强制将其基类指派为最新的物品实例基础类 USingularisItem
	return FKismetEditorUtilities::CreateBlueprint(
		USingularisItem::StaticClass(),
		InParent,
		InName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);
}

bool USingularisItemFactory::ShouldShowInNewMenu() const
{
	return true;
}

UClass* FAssetTypeActions_SingularisItem::GetSupportedClass() const
{
	return USingularisItem::StaticClass();
}
