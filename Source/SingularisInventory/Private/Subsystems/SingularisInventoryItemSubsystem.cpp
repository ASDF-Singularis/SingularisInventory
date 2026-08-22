#include "Subsystems/SingularisInventoryItemSubsystem.h"

#include <Engine/DataTable.h>

#include "SingularisInventory.h"
#include "Configs/SingularisInventorySettings.h"
#include "Objects/SingularisItem.h"

USingularisInventoryItemSubsystem::USingularisInventoryItemSubsystem() {}

void USingularisInventoryItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] Initialize：物品查询子系统初始化完成"),
		*GetNameSafe(this)
	);
}

void USingularisInventoryItemSubsystem::Deinitialize()
{
	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] Deinitialize：物品查询子系统卸载"),
		*GetNameSafe(this)
	);

	Super::Deinitialize();
}

UDataTable* USingularisInventoryItemSubsystem::GetItemTable() const
{
	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	if (!IsValid(Settings) || !IsValid(Settings->ItemTable.LoadSynchronous()))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品数据表无效，请在项目设置「Singularis → Singularis Inventory」中配置 ItemTable"));
		return nullptr;
	}
	return Settings->ItemTable.Get();
}

const FSingularisItemRow* USingularisInventoryItemSubsystem::FindItemRow(const USingularisItem* Item) const
{
	if (!IsValid(Item))
		return nullptr;

	return FindItemRowByClass(TSubclassOf<USingularisItem>(Item->GetClass()));
}

const FSingularisItemRow* USingularisInventoryItemSubsystem::FindItemRowByClass(
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

bool USingularisInventoryItemSubsystem::TryGetItemRow(USingularisItem* Item, FSingularisItemRow& OutRow) const
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

bool USingularisInventoryItemSubsystem::TryGetItemRowByClass(
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

TSubclassOf<AActor> USingularisInventoryItemSubsystem::GetFormActorClass(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->FormActorClass : nullptr;
}

TSubclassOf<AActor> USingularisInventoryItemSubsystem::GetFormActorClassByClass(
	const TSubclassOf<USingularisItem> ItemClass
) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->FormActorClass : nullptr;
}

UTexture2D* USingularisInventoryItemSubsystem::GetItemIcon(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Icon : nullptr;
}

UTexture2D* USingularisInventoryItemSubsystem::GetItemIconByClass(const TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Icon : nullptr;
}

FText USingularisInventoryItemSubsystem::GetItemName(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Name : FText{};
}

FText USingularisInventoryItemSubsystem::GetItemNameByClass(const TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Name : FText{};
}

FText USingularisInventoryItemSubsystem::GetItemDescription(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Description : FText{};
}

FText USingularisInventoryItemSubsystem::GetItemDescriptionByClass(const TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Description : FText{};
}
