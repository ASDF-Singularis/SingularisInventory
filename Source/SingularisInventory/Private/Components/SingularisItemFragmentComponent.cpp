#include "Components/SingularisItemFragmentComponent.h"

#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>
#include <GameFramework/Pawn.h>

#include "SingularisInventory.h"
#include "Components/SingularisInventoryComponent.h"
#include "Objects/SingularisItem.h"
#include "Objects/SingularisItemDefinition.h"
#include "Objects/SingularisItemFragment.h"
#include "Types/SingularisItemFragmentType.h"
#include "Types/SingularisItemType.h"

USingularisItemFragmentComponent::USingularisItemFragmentComponent()
{
	SetIsReplicatedByDefault(false);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisItemFragmentComponent::BeginPlay()
{
	Super::BeginPlay();

	checkf(
		GetOwner()->IsA<APawn>(),
		TEXT("[%s] Owner 非 Pawn（实际：%s）"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(GetOwner()->GetClass())
	);
}

void USingularisItemFragmentComponent::Execute(
	USingularisItem* Item,
	const FGameplayTag& FragmentTag,
	const FInputActionValue& InputValue
)
{
	// 1) 零信任校验：物品实例与片段标签必须有效
	if (!IsValid(Item) || !FragmentTag.IsValid())
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] Execute：入参非法（物品 %s，标签 %s）"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item),
			*FragmentTag.ToString()
		);
		return;
	}

	// 2) 经物品实例背引用的定义取片段映射
	USingularisItemDefinition* const Definition = Item->GetDefinition();
	if (!IsValid(Definition))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] Execute：物品 %s 无物品定义"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item)
		);
		return;
	}

	// 3) 从 Owner 推导并组装片段上下文
	FSingularisItemFragmentContext Context;
	Context.Controller = GetOwner()->GetInstigatorController();
	Context.Instigator = Cast<APawn>(GetOwner());
	Context.Avatar = GetOwner();
	Context.Item = Item;
	Context.Inventory = IsValid(Context.Controller)
		                    ? Context.Controller->FindComponentByClass<USingularisInventoryComponent>()
		                    : nullptr;
	Context.InputValue = InputValue;

	// 4) 标签层级匹配命中管线，逐片段执行
	for (const auto& [Tag, Pipeline] : Definition->FragmentMapping)
	{
		if (!Tag.MatchesTag(FragmentTag))
			continue;

		for (const FSingularisItemFragmentEntry& Entry : Pipeline.Fragments)
		{
			USingularisItemFragment* const Fragment = Entry.Fragment;
			if (!IsValid(Fragment))
				continue;

			Fragment->Trigger(Context);
		}
	}
}
