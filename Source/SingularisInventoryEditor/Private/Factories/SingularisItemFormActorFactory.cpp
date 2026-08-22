#include "Factories/SingularisItemFormActorFactory.h"

#include <Actors/SingularisItemFormActor.h>
#include <Kismet2/KismetEditorUtilities.h>

USingularisItemFormActorFactory::USingularisItemFormActorFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = ASingularisItemFormActor::StaticClass();
}

UObject* USingularisItemFormActorFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	const FName InName,
	const EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	// 1) 利用 KismetEditorUtilities 自动生成蓝图资产
	// 2) 强制将其基类指派为最新的物品形态基础类 ASingularisItemFormActor
	return FKismetEditorUtilities::CreateBlueprint(
		ASingularisItemFormActor::StaticClass(),
		InParent,
		InName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);
}

bool USingularisItemFormActorFactory::ShouldShowInNewMenu() const
{
	return true;
}

UClass* FAssetTypeActions_SingularisItemFormActor::GetSupportedClass() const
{
	return ASingularisItemFormActor::StaticClass();
}
