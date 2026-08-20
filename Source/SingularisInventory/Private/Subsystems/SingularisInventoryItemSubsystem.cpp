#include "Subsystems/SingularisInventoryItemSubsystem.h"

#include <Engine/DataTable.h>

#include "Configs/SingularisInventorySettings.h"
#include "Objects/SingularisItem.h"

USingularisInventoryItemSubsystem::USingularisInventoryItemSubsystem() {}

void USingularisInventoryItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USingularisInventoryItemSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UDataTable* USingularisInventoryItemSubsystem::GetItemTable() const
{
	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	return IsValid(Settings) ? Settings->ItemTable.Get() : nullptr;
}

const FSingularisItemRow* USingularisInventoryItemSubsystem::FindItemRow(const USingularisItem* Item) const
{
	if (!IsValid(Item))
		return nullptr;

	return FindItemRowByClass(TSubclassOf<USingularisItem>(Item->GetClass()));
}

const FSingularisItemRow* USingularisInventoryItemSubsystem::FindItemRowByClass(
	TSubclassOf<USingularisItem> ItemClass
) const
{
	UDataTable* ItemTable = GetItemTable();
	if (!IsValid(ItemTable) || !IsValid(ItemClass.Get()))
		return nullptr;

	const UClass* ItemClassPtr = ItemClass.Get();
	for (const auto& Pair : ItemTable->GetRowMap())
	{
		const auto Row = reinterpret_cast<const FSingularisItemRow*>(Pair.Value);
		if (IsValid(Row->ItemClass) && Row->ItemClass.Get() == ItemClassPtr)
			return Row;
	}

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
	TSubclassOf<USingularisItem> ItemClass,
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
	TSubclassOf<USingularisItem> ItemClass
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

UTexture2D* USingularisInventoryItemSubsystem::GetItemIconByClass(TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Icon : nullptr;
}

FText USingularisInventoryItemSubsystem::GetItemName(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Name : FText{};
}

FText USingularisInventoryItemSubsystem::GetItemNameByClass(TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Name : FText{};
}

FText USingularisInventoryItemSubsystem::GetItemDescription(USingularisItem* Item) const
{
	const FSingularisItemRow* Row = FindItemRow(Item);
	return Row != nullptr ? Row->Description : FText{};
}

FText USingularisInventoryItemSubsystem::GetItemDescriptionByClass(TSubclassOf<USingularisItem> ItemClass) const
{
	const FSingularisItemRow* Row = FindItemRowByClass(ItemClass);
	return Row != nullptr ? Row->Description : FText{};
}
