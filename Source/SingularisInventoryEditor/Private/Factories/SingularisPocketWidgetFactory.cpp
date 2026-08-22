#include "Factories/SingularisPocketWidgetFactory.h"

#include <WidgetBlueprint.h>
#include <Kismet2/KismetEditorUtilities.h>
#include <Widgets/SingularisPocketWidget.h>

USingularisPocketWidgetFactory::USingularisPocketWidgetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USingularisPocketWidget::StaticClass();
}

UObject* USingularisPocketWidgetFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	const FName InName,
	const EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	return FKismetEditorUtilities::CreateBlueprint(
		USingularisPocketWidget::StaticClass(),
		InParent,
		InName,
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		// 必须指定为 UWidgetBlueprint
		UWidgetBlueprintGeneratedClass::StaticClass(),
		// 必须指定生成的类类型
		NAME_None
	);
}

bool USingularisPocketWidgetFactory::ShouldShowInNewMenu() const
{
	return Super::ShouldShowInNewMenu();
}
