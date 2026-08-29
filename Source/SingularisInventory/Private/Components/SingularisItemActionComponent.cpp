#include "Components/SingularisItemActionComponent.h"

#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>
#include <GameFramework/Pawn.h>

#include "SingularisInventory.h"
#include "Components/SingularisInventoryComponent.h"
#include "DataTables/SingularisItemRow.h"
#include "Objects/SingularisItem.h"
#include "Objects/SingularisItemAction.h"
#include "Subsystems/SingularisInventorySubsystem.h"
#include "Types/SingularisItemActionType.h"
#include "Types/SingularisItemType.h"

USingularisItemActionComponent::USingularisItemActionComponent()
{
	SetIsReplicatedByDefault(false);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisItemActionComponent::BeginPlay()
{
	Super::BeginPlay();

	checkf(
		GetOwner()->IsA<APawn>(),
		TEXT("[%s] Owner 非 Pawn（实际：%s）"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(GetOwner()->GetClass())
	);
}

void USingularisItemActionComponent::TryAction(
	USingularisItem* Item,
	const FGameplayTag& ActionTag,
	const FInputActionValue& InputValue
)
{
	// 1) 零信任校验：物品实例与动作标签必须有效
	if (!IsValid(Item) || !ActionTag.IsValid())
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] TryAction：入参非法（物品 %s，标签 %s）"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item),
			*ActionTag.ToString()
		);
		return;
	}

	// 2) 查数据表行，取动作映射
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(World->GetGameInstance()))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] TryAction：World 或 GameInstance 无效"),
			*GetNameSafe(GetOwner())
		);
		return;
	}
	const USingularisInventorySubsystem* Subsystem =
		World->GetGameInstance()->GetSubsystem<USingularisInventorySubsystem>();
	if (!IsValid(Subsystem))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] TryAction：物品查询子系统无效"), *GetNameSafe(GetOwner()));
		return;
	}
	FSingularisItemRow Row;
	if (!Subsystem->TryGetItemRow(Item, Row))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] TryAction：物品 %s 未在数据表中找到行"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item)
		);
		return;
	}

	// 3) 从 Owner 推导并组装动作上下文
	FSingularisItemActionContext Context;
	Context.Controller = GetOwner()->GetInstigatorController();
	Context.Instigator = Cast<APawn>(GetOwner());
	Context.Avatar = GetOwner();
	Context.Item = Item;
	Context.Inventory = IsValid(Context.Controller)
		                    ? Context.Controller->FindComponentByClass<USingularisInventoryComponent>()
		                    : nullptr;
	Context.InputValue = InputValue;

	// 4) 标签层级匹配命中管线，逐动作执行
	for (const auto& [Tag, Pipeline] : Row.ItemActionMapping)
	{
		if (!Tag.MatchesTag(ActionTag))
			continue;

		for (const FSingularisItemActionEntry& Entry : Pipeline.Actions)
		{
			USingularisItemAction* const Action = Entry.Action;
			if (!IsValid(Action))
				continue;

			Action->Trigger(Context);
		}
	}
}
