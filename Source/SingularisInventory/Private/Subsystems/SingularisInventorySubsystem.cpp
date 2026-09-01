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
#include "DataTables/SingularisItemFormRow.h"
#include "Objects/SingularisItem.h"
#include "Objects/SingularisItemDefinition.h"

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
	FormActorToTagMap.Empty();
	TagToDefinitionMap.Empty();

	Super::Deinitialize();
}

USingularisItemDefinition* USingularisInventorySubsystem::GetItemDefinition(USingularisItem* Item) const
{
	return IsValid(Item) ? Item->GetDefinition() : nullptr;
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
			TEXT("物品标签 %s 未在物品形态注册表中找到形态 Actor 类"),
			*ItemTag.ToString()
		);
		return nullptr;
	}
	return *FormActorClass;
}

FGameplayTag USingularisInventorySubsystem::FindItemTagByFormActorClass(const TSubclassOf<AActor> FormActorClass) const
{
	const FGameplayTag* const ItemTag = FormActorToTagMap.Find(FormActorClass);
	return ItemTag != nullptr ? *ItemTag : FGameplayTag{};
}

bool USingularisInventorySubsystem::RegisterItemForm(
	const FGameplayTag& ItemTag,
	const TSubclassOf<AActor> FormActorClass
)
{
	// 1) 零信任校验
	if (!ItemTag.IsValid() || !IsValid(FormActorClass))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] RegisterItemForm：入参非法（标签 %s，形态 %s）"),
			*GetNameSafe(this),
			*ItemTag.ToString(),
			*GetNameSafe(FormActorClass)
		);
		return false;
	}

	// 2) 幂等：同一对已注册则无副作用
	if (const TSubclassOf<AActor>* const ExistingActor = TagToFormActorMap.Find(ItemTag))
	{
		if (*ExistingActor == FormActorClass)
			return true;
	}

	// 3) 拆除旧关联，保证双向映射一致
	if (const TSubclassOf<AActor>* const ExistingActor = TagToFormActorMap.Find(ItemTag))
		FormActorToTagMap.Remove(*ExistingActor);
	if (const FGameplayTag* const ExistingTag = FormActorToTagMap.Find(FormActorClass))
		TagToFormActorMap.Remove(*ExistingTag);

	// 4) 写入双向映射
	TagToFormActorMap.Add(ItemTag, FormActorClass);
	FormActorToTagMap.Add(FormActorClass, ItemTag);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] RegisterItemForm：%s -> %s 注册成功"),
		*GetNameSafe(this),
		*ItemTag.ToString(),
		*GetNameSafe(FormActorClass)
	);
	return true;
}

bool USingularisInventorySubsystem::UnregisterItemForm(const FGameplayTag& ItemTag)
{
	const TSubclassOf<AActor>* const ExistingActor = TagToFormActorMap.Find(ItemTag);
	if (ExistingActor == nullptr)
		return false;

	FormActorToTagMap.Remove(*ExistingActor);
	TagToFormActorMap.Remove(ItemTag);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] UnregisterItemForm：%s 注销成功"),
		*GetNameSafe(this),
		*ItemTag.ToString()
	);
	return true;
}

void USingularisInventorySubsystem::RebuildRegistry()
{
	// 1) 清空旧映射，避免残留脏数据
	TagToFormActorMap.Empty();
	FormActorToTagMap.Empty();
	TagToDefinitionMap.Empty();

	// 2) 从物品形态注册表构建 ItemTag <-> FormActorClass 双向映射
	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	UDataTable* const FormTable = IsValid(Settings) ? Settings->ItemFormTable.LoadSynchronous() : nullptr;
	if (!IsValid(FormTable))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品形态注册表无效，请在项目设置中配置 ItemFormTable"));
	}
	else
	{
		for (const auto& [RowName, RowPtr] : FormTable->GetRowMap())
		{
			const auto Row = reinterpret_cast<const FSingularisItemFormRow*>(RowPtr);
			if (Row == nullptr || !IsValid(Row->FormActorClass))
				continue;

			const FGameplayTag ItemTag = FGameplayTag::RequestGameplayTag(RowName, false);
			if (!ItemTag.IsValid())
			{
				UE_LOG(
					LogSingularisInventory,
					Warning,
					TEXT("物品形态注册表：行名 %s 不是有效 GameplayTag，已跳过"),
					*RowName.ToString()
				);
				continue;
			}

			TagToFormActorMap.Add(ItemTag, Row->FormActorClass);
			FormActorToTagMap.Add(Row->FormActorClass, ItemTag);
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

	// 3) 经物品实例背引用的定义取物品标签，再经形态表查形态 Actor 类
	USingularisItemDefinition* const Definition = GetItemDefinition(Item);
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
	const TSubclassOf<AActor> FormActorClass = FindFormActorClass(Definition->ItemTag);
	if (!IsValid(FormActorClass))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品 %s 未在形态表中配置形态 Actor 类"),
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
