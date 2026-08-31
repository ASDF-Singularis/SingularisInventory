#include "Subsystems/SingularisInventorySubsystem.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/EngineTypes.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include "SingularisInventory.h"
#include "Components/SingularisItemComponent.h"
#include "Configs/SingularisInventorySettings.h"
#include "Objects/SingularisItem.h"
#include "Objects/SingularisItemDefinition.h"

USingularisInventorySubsystem::USingularisInventorySubsystem() {}

void USingularisInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] Initialize：物品查询子系统初始化完成"),
		*GetNameSafe(this)
	);
}

void USingularisInventorySubsystem::Deinitialize()
{
	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] Deinitialize：物品查询子系统卸载"),
		*GetNameSafe(this)
	);

	Super::Deinitialize();
}

USingularisItemDefinition* USingularisInventorySubsystem::GetItemDefinition(USingularisItem* Item) const
{
	return IsValid(Item) ? Item->GetDefinition() : nullptr;
}

USingularisItemDefinition* USingularisInventorySubsystem::FindDefinitionByFormActorClass(
	const TSubclassOf<AActor> FormActorClass
) const
{
	// 1) 零信任校验：形态 Actor 类必须有效
	if (!IsValid(FormActorClass))
		return nullptr;

	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	if (!IsValid(Settings))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品定义注册表设置无效"));
		return nullptr;
	}

	// 2) 扫描注册表匹配形态 Actor 类
	const UClass* const FormActorClassPtr = FormActorClass.Get();
	for (const TSoftObjectPtr<USingularisItemDefinition>& DefinitionRef : Settings->ItemDefinitions)
	{
		USingularisItemDefinition* const Definition = DefinitionRef.LoadSynchronous();
		if (IsValid(Definition) && IsValid(Definition->FormActorClass) && Definition->FormActorClass.Get() == FormActorClassPtr)
			return Definition;
	}

	UE_LOG(
		LogSingularisInventory,
		Warning,
		TEXT("形态 Actor 类 %s 未在物品定义注册表中找到，请检查物品定义配置"),
		*GetNameSafe(FormActorClassPtr)
	);
	return nullptr;
}

AActor* USingularisInventorySubsystem::SpawnItemInWorld(USingularisItem* Item, const FTransform& Transform) const
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：物品实例无效"), *GetNameSafe(this));
		return nullptr;
	}

	// 2) 取 World（本子系统经 GameInstance 解析，无需调用方传入上下文）
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：World 无效"), *GetNameSafe(this));
		return nullptr;
	}

	// 3) 经物品实例背引用的定义查形态 Actor 类
	USingularisItemDefinition* const Definition = GetItemDefinition(Item);
	const TSubclassOf<AActor> FormActorClass = IsValid(Definition) ? Definition->FormActorClass : nullptr;
	if (!IsValid(FormActorClass))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品 %s 未配置形态 Actor 类"),
			*GetNameSafe(this),
			*GetNameSafe(Item)
		);
		return nullptr;
	}

	// 4) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = World->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：形态 Actor %s 生成失败"),
			*GetNameSafe(this),
			*GetNameSafe(FormActorClass)
		);
		return nullptr;
	}

	// 5) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
		ItemComponent->BindItem(Item);
	else
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] SpawnItemInWorld：形态 Actor %s 无 ItemComponent，物品 %s 仅入世不可收容"),
			*GetNameSafe(this),
			*GetNameSafe(FormActor),
			*GetNameSafe(Item)
		);

	// 6) 开启物理
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(FormActor->GetRootComponent()))
	{
		PrimitiveComponent->SetMobility(EComponentMobility::Movable);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PrimitiveComponent->SetSimulatePhysics(true);

		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] SpawnItemInWorld：形态 Actor %s 开启物理"),
			*GetNameSafe(this),
			*GetNameSafe(FormActor)
		);
	}

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] SpawnItemInWorld：物品 %s(%s) 生成入世界成功"),
		*GetNameSafe(this),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
	return FormActor;
}

USingularisItem* USingularisInventorySubsystem::CollectItem(AActor* FormActor) const
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] CollectItem：形态 Actor 无效"), *GetNameSafe(this));
		return nullptr;
	}

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] CollectItem：形态 Actor %s 无 ItemComponent"),
			*GetNameSafe(this),
			*GetNameSafe(FormActor)
		);
		return nullptr;
	}

	// 3) 取回物品实例；无物品则不销毁形态 Actor
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] CollectItem：形态 Actor %s 无物品可收容"),
			*GetNameSafe(this),
			*GetNameSafe(FormActor)
		);
		return nullptr;
	}

	// 4) 销毁形态 Actor，返回物品实例（容器路由由调用方 / PickupItem 负责）
	FormActor->Destroy();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] CollectItem：物品 %s(%s) 收容成功，形态 Actor 已销毁"),
		*GetNameSafe(this),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
	return Item;
}
