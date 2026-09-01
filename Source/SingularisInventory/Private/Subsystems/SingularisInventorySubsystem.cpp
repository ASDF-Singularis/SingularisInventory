#include "Subsystems/SingularisInventorySubsystem.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/AssetManager.h>
#include <Engine/DataTable.h>
#include <Engine/EngineTypes.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include "SingularisInventory.h"
#include "Components/SingularisItemComponent.h"
#include "Configs/SingularisInventorySettings.h"
#include "DataAssets/SingularisItemDefinition.h"
#include "DataTables/SingularisItemFormRow.h"
#include "Objects/SingularisItem.h"

USingularisInventorySubsystem::USingularisInventorySubsystem() {}

void USingularisInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RebuildRegistry();

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

	TagToFormActorMap.Empty();
	TagToDefinitionMap.Empty();
	DefinitionToFormActorMap.Empty();
	FormActorToDefinitionMap.Empty();

	Super::Deinitialize();
}

USingularisItemDefinition* USingularisInventorySubsystem::FindDefinitionByItemTag(const FGameplayTag& ItemTag) const
{
	const TObjectPtr<USingularisItemDefinition>* const Definition = TagToDefinitionMap.Find(ItemTag);
	if (Definition == nullptr)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("物品标签 %s 未映射到物品定义资产，请检查定义资产与 AssetManager 扫描配置"),
			*ItemTag.ToString()
		);
		return nullptr;
	}
	return Definition->Get();
}

TSubclassOf<AActor> USingularisInventorySubsystem::FindFormActorClass(const FGameplayTag& ItemTag) const
{
	const TSubclassOf<AActor>* const FormActorClass = TagToFormActorMap.Find(ItemTag);
	if (FormActorClass == nullptr)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("物品标签 %s 未在物品形态注册表中找到对应物品形态"),
			*ItemTag.ToString()
		);
		return nullptr;
	}
	return *FormActorClass;
}

TSubclassOf<AActor> USingularisInventorySubsystem::FindFormActorClassByDefinition(
	USingularisItemDefinition* Definition
) const
{
	// 1) 零信任校验
	if (!IsValid(Definition))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品定义无效，无法查询物品形态"));
		return nullptr;
	}

	// 2) 查定义 -> 形态映射
	const TSubclassOf<AActor>* const FormActorClass = DefinitionToFormActorMap.Find(Definition);
	if (FormActorClass == nullptr)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("物品定义 %s 未映射到物品形态，请检查物品形态注册表"),
			*GetNameSafe(Definition)
		);
		return nullptr;
	}
	return *FormActorClass;
}

USingularisItemDefinition* USingularisInventorySubsystem::FindDefinitionByFormActorClass(
	const TSubclassOf<AActor> FormActorClass
) const
{
	const TObjectPtr<USingularisItemDefinition>* const Definition = FormActorToDefinitionMap.Find(FormActorClass);
	return Definition != nullptr ? Definition->Get() : nullptr;
}

bool USingularisInventorySubsystem::RegisterItemForm(
	USingularisItemDefinition* Definition,
	const TSubclassOf<AActor> FormActorClass
)
{
	// 1) 零信任校验
	if (!IsValid(Definition) || !Definition->ItemTag.IsValid() || !IsValid(FormActorClass))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] RegisterItemForm：入参非法（定义 %s，物品形态 %s）"),
			*GetNameSafe(this),
			*GetNameSafe(Definition),
			*GetNameSafe(FormActorClass)
		);
		return false;
	}

	const FGameplayTag ItemTag = Definition->ItemTag;

	// 2) 幂等：同一关联已注册则无副作用
	if (const TSubclassOf<AActor>* const ExistingForm = DefinitionToFormActorMap.Find(Definition))
	{
		if (*ExistingForm == FormActorClass)
			return true;
	}

	// 3) 拆除新形态的旧标签关联，保证标签映射唯一
	if (const TObjectPtr<USingularisItemDefinition>* const ExistingDefinition = FormActorToDefinitionMap.Find(
		FormActorClass
	))
	{
		const FGameplayTag ExistingTag = (*ExistingDefinition)->ItemTag;
		if (const TSubclassOf<AActor>* const TaggedForm = TagToFormActorMap.Find(ExistingTag))
		{
			if (*TaggedForm == FormActorClass)
				TagToFormActorMap.Remove(ExistingTag);
		}
	}

	// 4) 写入标签映射，重建定义-形态双向映射
	TagToFormActorMap.Add(ItemTag, FormActorClass);
	TagToDefinitionMap.Add(ItemTag, Definition);
	RebuildDefinitionFormMaps();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] RegisterItemForm：%s -> %s 注册成功"),
		*GetNameSafe(this),
		*GetNameSafe(Definition),
		*GetNameSafe(FormActorClass)
	);
	return true;
}

bool USingularisInventorySubsystem::UnregisterItemForm(USingularisItemDefinition* Definition)
{
	// 1) 零信任校验
	if (!IsValid(Definition) || !Definition->ItemTag.IsValid())
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] UnregisterItemForm：入参非法（定义 %s）"),
			*GetNameSafe(this),
			*GetNameSafe(Definition)
		);
		return false;
	}

	// 2) 未注册则无副作用
	const TSubclassOf<AActor>* const ExistingForm = DefinitionToFormActorMap.Find(Definition);
	if (ExistingForm == nullptr)
		return false;

	// 3) 拆除标签 -> 形态关联并重建双向映射（物品定义映射保持可查）
	if (const TSubclassOf<AActor>* const TaggedForm = TagToFormActorMap.Find(Definition->ItemTag))
	{
		if (*TaggedForm == *ExistingForm)
			TagToFormActorMap.Remove(Definition->ItemTag);
	}
	RebuildDefinitionFormMaps();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] UnregisterItemForm：%s 注销成功"),
		*GetNameSafe(this),
		*GetNameSafe(Definition)
	);
	return true;
}

void USingularisInventorySubsystem::RebuildRegistry()
{
	// 1) 清空旧映射，避免残留脏数据
	TagToFormActorMap.Empty();
	TagToDefinitionMap.Empty();
	DefinitionToFormActorMap.Empty();
	FormActorToDefinitionMap.Empty();

	// 2) 从物品形态注册表构建 ItemTag -> FormActorClass 映射
	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	UDataTable* const FormTable = IsValid(Settings) ? Settings->ItemFormTable.LoadSynchronous() : nullptr;
	if (!IsValid(FormTable))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品形态注册表无效，请在项目设置中配置 ItemFormTable"));
	}
	else
	{
		for (const auto& Pair : FormTable->GetRowMap())
		{
			const auto Row = reinterpret_cast<const FSingularisItemFormRow*>(Pair.Value);
			if (Row == nullptr || !Row->ItemTag.IsValid() || !IsValid(Row->FormActorClass))
				continue;

			TagToFormActorMap.Add(Row->ItemTag, Row->FormActorClass);
		}

		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] RebuildRegistry：物品形态注册表已载入 %d 条映射"),
			*GetNameSafe(this),
			TagToFormActorMap.Num()
		);
	}

	// 3) 经 AssetManager 扫描物品定义资产，构建 ItemTag -> Definition 映射
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(USingularisItemDefinition::ItemType, AssetIds);
	AssetManager.LoadPrimaryAssets(AssetIds);

	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		USingularisItemDefinition* const Definition =
			AssetManager.GetPrimaryAssetObject<USingularisItemDefinition>(AssetId);
		if (IsValid(Definition) && Definition->ItemTag.IsValid())
			TagToDefinitionMap.Add(Definition->ItemTag, Definition);
	}

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] RebuildRegistry：物品定义映射已载入 %d 条"),
		*GetNameSafe(this),
		TagToDefinitionMap.Num()
	);

	// 4) 经标签桥接推导 Definition <-> FormActorClass 双向映射
	RebuildDefinitionFormMaps();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] RebuildRegistry：定义-形态映射已推导 %d 条"),
		*GetNameSafe(this),
		DefinitionToFormActorMap.Num()
	);
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

	// 3) 经物品实例背引用的定义查形态映射表取物品形态
	USingularisItemDefinition* const Definition = Item->GetDefinition();
	if (!IsValid(Definition))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品 %s 无物品定义"),
			*GetNameSafe(this),
			*GetNameSafe(Item)
		);
		return nullptr;
	}
	const TSubclassOf<AActor> FormActorClass = FindFormActorClassByDefinition(Definition);
	if (!IsValid(FormActorClass))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品 %s 未映射到对应物品形态"),
			*GetNameSafe(this),
			*GetNameSafe(Item)
		);
		return nullptr;
	}

	// 4) 生成物品形态
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = World->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品形态 %s 生成失败"),
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
		TEXT("[%s] SpawnItemInWorld：物品形态 %s 无 ItemComponent，物品 %s 仅入世不可收容"),
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
			TEXT("[%s] SpawnItemInWorld：物品形态 %s 开启物理"),
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
	// 1) 零信任校验：物品形态必须有效
	if (!IsValid(FormActor))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] CollectItem：物品形态无效"), *GetNameSafe(this));
		return nullptr;
	}

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] CollectItem：物品形态 %s 无 ItemComponent"),
			*GetNameSafe(this),
			*GetNameSafe(FormActor)
		);
		return nullptr;
	}

	// 3) 取回物品实例；无物品则不销毁物品形态
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] CollectItem：物品形态 %s 无物品可收容"),
			*GetNameSafe(this),
			*GetNameSafe(FormActor)
		);
		return nullptr;
	}

	// 4) 销毁物品形态，返回物品实例（容器路由由调用方 / PickupItem 负责）
	FormActor->Destroy();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] CollectItem：物品 %s(%s) 收容成功，物品形态已销毁"),
		*GetNameSafe(this),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
	return Item;
}

void USingularisInventorySubsystem::RebuildDefinitionFormMaps()
{
	DefinitionToFormActorMap.Empty();
	FormActorToDefinitionMap.Empty();

	for (const auto& Pair : TagToFormActorMap)
	{
		const TObjectPtr<USingularisItemDefinition>* const Definition = TagToDefinitionMap.Find(Pair.Key);
		if (Definition == nullptr)
			continue;

		DefinitionToFormActorMap.Add(*Definition, Pair.Value);
		FormActorToDefinitionMap.Add(Pair.Value, *Definition);
	}
}
