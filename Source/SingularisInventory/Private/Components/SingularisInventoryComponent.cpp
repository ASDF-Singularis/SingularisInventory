#include "Components/SingularisInventoryComponent.h"

#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include "Components/SingularisItemComponent.h"
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "SingularisInventory.h"
#include "Subsystems/SingularisInventoryItemSubsystem.h"

USingularisInventoryComponent::USingularisInventoryComponent()
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

AActor* USingularisInventoryComponent::SpawnItemInWorld(USingularisItem* Item, FTransform Transform)
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：物品实例为空"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 世界必须有效
	UWorld* World = GetWorld();
	if (!ensureMsgf(IsValid(World), TEXT("[%s] SpawnItemInWorld：World 无效"), *GetNameSafe(GetOwner())))
		return nullptr;

	// 3) 经全局查询子系统取物品形态 Actor 类
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!ensureMsgf(IsValid(GameInstance), TEXT("[%s] SpawnItemInWorld：GameInstance 无效"), *GetNameSafe(GetOwner())))
		return nullptr;
	const USingularisInventoryItemSubsystem* ItemSubsystem = GameInstance->GetSubsystem<
		USingularisInventoryItemSubsystem>();
	if (!ensureMsgf(IsValid(ItemSubsystem), TEXT("[%s] SpawnItemInWorld：物品查询子系统无效"), *GetNameSafe(GetOwner())))
		return nullptr;
	const TSubclassOf<AActor> FormActorClass = ItemSubsystem->GetFormActorClass(Item);
	if (!IsValid(FormActorClass))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：物品类 %s 未配置形态 Actor，请在数据表中补充行"), *GetNameSafe(GetOwner()), *GetNameSafe(Item->GetClass()));
		return nullptr;
	}

	// 4) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = World->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!ensureMsgf(IsValid(FormActor), TEXT("[%s] SpawnItemInWorld：生成形态 Actor %s 失败"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActorClass.Get())))
		return nullptr;

	// 5) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
	{
		ItemComponent->BindItem(Item);
	}
	else
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SpawnItemInWorld：形态 Actor %s 无 ItemComponent，仅入世不可收容"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActor));
	}

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SpawnItemInWorld：物品 %s(%s) 入世成功 → 形态 Actor %s（位置 %s）"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()), *GetNameSafe(FormActor), *Transform.GetLocation().ToString());
	return FormActor;
}

USingularisItem* USingularisInventoryComponent::CollectItem(
	AActor* FormActor,
	USingularisPocketComponent* TargetContainer
)
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] CollectItem：形态 Actor 为空"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：形态 Actor %s 无 ItemComponent，不可收容"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActor));
		return nullptr;
	}

	// 3) 取回物品实例；无物品则不销毁形态 Actor
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：形态 Actor %s 无物品"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActor));
		return nullptr;
	}

	// 4) 销毁形态 Actor
	FormActor->Destroy();

	// 5) 提供目标容器则尝试入容器；满或未提供容器时返回实例由调用方处置
	if (IsValid(TargetContainer))
	{
		const int32 SlotIndex = TargetContainer->AddItem(Item);
		if (SlotIndex != INDEX_NONE)
		{
			UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：物品 %s(%s) 收回成功，入容器 %s 插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()), *GetNameSafe(TargetContainer->GetOwner()), SlotIndex);
		}
		else
		{
			UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：物品 %s(%s) 收回成功，容器已满未放入"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()));
		}
	}
	else
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：物品 %s(%s) 收回成功，未提供容器"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()));
	}

	return Item;
}
