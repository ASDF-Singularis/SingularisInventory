#include "Subsystems/SingularisInventorySubsystem.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/DataTable.h>
#include <Engine/EngineTypes.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include "SingularisInventory.h"
#include "Components/SingularisItemComponent.h"
#include "Configs/SingularisInventorySettings.h"
#include "Objects/SingularisItem.h"

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

UDataTable* USingularisInventorySubsystem::GetItemTable() const
{
	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	if (!IsValid(Settings) || !IsValid(Settings->ItemTable.LoadSynchronous()))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品数据表无效，请在项目设置「Singularis → Singularis Inventory」中配置 ItemTable"));
		return nullptr;
	}
	return Settings->ItemTable.Get();
}

bool USingularisInventorySubsystem::TryGetItemRow(USingularisItem* Item, FSingularisItemRow& OutRow) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	if (Row == nullptr)
	{
		OutRow = FSingularisItemRow{};
		return false;
	}
	OutRow = *Row;
	return true;
}

bool USingularisInventorySubsystem::TryGetItemRowByClass(
	const TSubclassOf<USingularisItem> ItemClass,
	FSingularisItemRow& OutRow
) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	if (Row == nullptr)
	{
		OutRow = FSingularisItemRow{};
		return false;
	}
	OutRow = *Row;
	return true;
}

bool USingularisInventorySubsystem::TryGetItemRowByFormActorClass(
	const TSubclassOf<AActor> FormActorClass,
	FSingularisItemRow& OutRow
) const
{
	const FSingularisItemRow* Row = FindItemRowByFormActorClass(FormActorClass);
	if (Row == nullptr)
	{
		OutRow = FSingularisItemRow{};
		return false;
	}
	OutRow = *Row;
	return true;
}

TSubclassOf<AActor> USingularisInventorySubsystem::GetFormActorClass(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->FormActorClass : nullptr;
}

TSubclassOf<AActor> USingularisInventorySubsystem::GetFormActorClassByClass(
	const TSubclassOf<USingularisItem> ItemClass
) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->FormActorClass : nullptr;
}

UTexture2D* USingularisInventorySubsystem::GetItemIcon(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Icon : nullptr;
}

UTexture2D* USingularisInventorySubsystem::GetItemIconByClass(const TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Icon : nullptr;
}

FText USingularisInventorySubsystem::GetItemName(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Name : FText{};
}

FText USingularisInventorySubsystem::GetItemNameByClass(const TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Name : FText{};
}

FText USingularisInventorySubsystem::GetItemDescription(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Description : FText{};
}

FText USingularisInventorySubsystem::GetItemDescriptionByClass(const TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Description : FText{};
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

	// 3) 查物品形态 Actor 类（本子系统内直接查行）
	const TSubclassOf<AActor> FormActorClass = GetFormActorClass(Item);
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

const FSingularisItemRow* USingularisInventorySubsystem::FindItemRow(const USingularisItem* Item) const
{
	if (!IsValid(Item))
		return nullptr;

	return FindItemRowByClass(TSubclassOf<USingularisItem>(Item->GetClass()));
}

const FSingularisItemRow* USingularisInventorySubsystem::FindItemRowByClass(
	const TSubclassOf<USingularisItem> ItemClass
) const
{
	UDataTable* ItemTable = GetItemTable();
	if (!IsValid(ItemTable) || !IsValid(ItemClass.Get()))
		return nullptr; // 表无效时 GetItemTable 已记录日志；空入参不记

	const UClass* ItemClassPtr = ItemClass.Get();
	for (const auto& Pair : ItemTable->GetRowMap())
	{
		const auto Row = reinterpret_cast<const FSingularisItemRow*>(Pair.Value);
		if (IsValid(Row->ItemClass) && Row->ItemClass.Get() == ItemClassPtr)
			return Row;
	}

	UE_LOG(LogSingularisInventory, Warning, TEXT("物品类 %s 未在数据表中找到行，请检查物品数据配置"), *GetNameSafe(ItemClass.Get()));
	return nullptr;
}

const FSingularisItemRow* USingularisInventorySubsystem::FindItemRowByFormActorClass(
	const TSubclassOf<AActor> FormActorClass
) const
{
	UDataTable* ItemTable = GetItemTable();
	if (!IsValid(ItemTable) || !IsValid(FormActorClass.Get()))
		return nullptr; // 表无效时 GetItemTable 已记录日志；空入参不记

	const UClass* FormActorClassPtr = FormActorClass.Get();
	for (const auto& Pair : ItemTable->GetRowMap())
	{
		const auto Row = reinterpret_cast<const FSingularisItemRow*>(Pair.Value);
		if (IsValid(Row->FormActorClass) && Row->FormActorClass.Get() == FormActorClassPtr)
			return Row;
	}

	UE_LOG(
		LogSingularisInventory,
		Warning,
		TEXT("形态 Actor 类 %s 未在数据表中找到行，请检查物品数据配置"),
		*GetNameSafe(FormActorClass.Get())
	);
	return nullptr;
}
