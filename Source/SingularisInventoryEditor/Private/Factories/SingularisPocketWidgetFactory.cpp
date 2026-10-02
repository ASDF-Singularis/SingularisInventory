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
	// 1) 利用 KismetEditorUtilities 自动生成控件蓝图资产
	// 2) 指定 UWidgetBlueprint 蓝图类型与 UWidgetBlueprintGeneratedClass 生成类类型
	return FKismetEditorUtilities::CreateBlueprint(
		USingularisPocketWidget::StaticClass(),
		InParent,
		InName,
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);
}

bool USingularisPocketWidgetFactory::ShouldShowInNewMenu() const
{
	return Super::ShouldShowInNewMenu();
}
