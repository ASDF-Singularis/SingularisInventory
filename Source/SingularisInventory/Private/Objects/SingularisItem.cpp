#include "Objects/SingularisItem.h"

#include <Engine/Engine.h>
#include <GameFramework/PlayerController.h>
#include <UObject/UObjectGlobals.h>

#include "Components/SingularisInventoryComponent.h"
#include "Objects/SingularisItemAction.h"
#include "Types/SingularisItemActionType.h"

USingularisItem* USingularisItem::MaterializeFromTemplate(UObject* Outer, const USingularisItem* Template)
{
	// 1) 零信任校验：Outer 与模板必须有效
	if (!IsValid(Outer) || !IsValid(Template))
		return nullptr;

	// 2) 按模板类在新 Outer 下创建独立实例，复制模板属性使其脱离模板引用关系
	USingularisItem* const Materialized = NewObject<USingularisItem>(Outer, Template->GetClass());
	UEngine::CopyPropertiesForUnrelatedObjects(const_cast<USingularisItem*>(Template), Materialized);

	return Materialized;
}

void USingularisItem::TryAction(
	const FGameplayTag& ActionTag,
	AController* Controller,
	APawn* Instigator,
	AActor* Avatar,
	const FInputActionValue& InputActionValue
)
{
	// 1) 零信任校验：控制器与动作标签必须有效
	if (!IsValid(Controller) || !ActionTag.IsValid())
		return;

	// 2) 组装动作上下文
	FSingularisItemActionContext Context;
	Context.Controller = Controller;
	Context.Instigator = Instigator;
	Context.Avatar = Avatar;
	Context.Item = this;
	Context.Inventory = Controller->FindComponentByClass<USingularisInventoryComponent>();
	Context.InputValue = InputActionValue;

	// 3) 标签层级匹配命中管线，逐动作执行；bSuspend 时失败中断后续
	for (const auto& [Tag, Pipeline] : ItemActionMapping)
	{
		if (!Tag.MatchesTag(ActionTag))
			continue;

		for (const FSingularisItemActionEntry& Entry : Pipeline.Actions)
		{
			USingularisItemAction* const Action = Entry.Action;
			if (!IsValid(Action))
				continue;

			Action->Execute(Context);
		}
	}
}
