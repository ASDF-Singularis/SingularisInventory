#include "Components/SingularisItemFragmentComponent.h"

#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>
#include <GameFramework/Pawn.h>

#include "SingularisInventory.h"
#include "Components/SingularisInventoryComponent.h"
#include "DataAssets/SingularisItemDefinition.h"
#include "Objects/SingularisItem.h"
#include "Objects/SingularisItemFragment.h"
#include "Types/SingularisItemFragmentType.h"

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

	// 2) 经物品实例背引用的定义取平铺片段数组
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

	// 4) 遍历定义平铺片段数组，按片段自报标签层级匹配并逐片段执行
	FGameplayTagContainer FragmentTags;
	for (USingularisItemFragment* const Fragment : Definition->Fragments)
	{
		if (!IsValid(Fragment))
			continue;

		FragmentTags.Reset();
		Fragment->GetOwnedGameplayTags(FragmentTags);
		if (!FragmentTags.HasTag(FragmentTag))
			continue;

		Fragment->Trigger(Context);
	}
}
