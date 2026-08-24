#include "Components/SingularisInventoryComponent.h"

#include <EnhancedInputComponent.h>
#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>
#include <Engine/GameInstance.h>
#include <Engine/LocalPlayer.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Character.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>

#include "SingularisInventory.h"
#include "Components/SingularisItemComponent.h"
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "Subsystems/SingularisInventoryItemSubsystem.h"

USingularisInventoryComponent::USingularisInventoryComponent()
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;

	static const ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContextFinder(
		TEXT("/SingularisInventory/Inputs/IMC_Singularis_Inventory.IMC_Singularis_Inventory")
	);
	static const ConstructorHelpers::FObjectFinder<UInputAction> DropActionFinder(
		TEXT("/SingularisInventory/Inputs/Actions/IA_Drop.IA_Drop")
	);
	static const ConstructorHelpers::FObjectFinder<UInputAction> FirstPocketActionFinder(
		TEXT("/SingularisInventory/Inputs/Actions/IA_FirstPocket.IA_FirstPocket")
	);
	static const ConstructorHelpers::FObjectFinder<UInputAction> SecondPocketActionFinder(
		TEXT("/SingularisInventory/Inputs/Actions/IA_SecondPocket.IA_SecondPocket")
	);
	static const ConstructorHelpers::FObjectFinder<UInputAction> ThirdPocketActionFinder(
		TEXT("/SingularisInventory/Inputs/Actions/IA_ThirdPocket.IA_ThirdPocket")
	);
	static const ConstructorHelpers::FObjectFinder<UInputAction> FourthPocketActionFinder(
		TEXT("/SingularisInventory/Inputs/Actions/IA_FourthPocket.IA_FourthPocket")
	);

	if (InputMappingContextFinder.Succeeded())
		InputMappingContext = InputMappingContextFinder.Object;
	else
		UE_LOG(
		LogSingularisInventory,
		Error,
		TEXT("默认输入映射上下文加载失败：%s"),
		TEXT("/SingularisInventory/Inputs/IMC_Singularis_Inventory")
	);

	if (DropActionFinder.Succeeded())
		DropInputAction = DropActionFinder.Object;
	else
		UE_LOG(
		LogSingularisInventory,
		Error,
		TEXT("默认丢弃输入动作加载失败：%s"),
		TEXT("/SingularisInventory/Inputs/Actions/IA_Drop")
	);

	if (FirstPocketActionFinder.Succeeded())
		SelectSlotActions.Add(FirstPocketActionFinder.Object);
	else
		UE_LOG(
		LogSingularisInventory,
		Error,
		TEXT("默认插槽 0 输入动作加载失败：%s"),
		TEXT("/SingularisInventory/Inputs/Actions/IA_FirstPocket")
	);

	if (SecondPocketActionFinder.Succeeded())
		SelectSlotActions.Add(SecondPocketActionFinder.Object);
	else
		UE_LOG(
		LogSingularisInventory,
		Error,
		TEXT("默认插槽 1 输入动作加载失败：%s"),
		TEXT("/SingularisInventory/Inputs/Actions/IA_SecondPocket")
	);

	if (ThirdPocketActionFinder.Succeeded())
		SelectSlotActions.Add(ThirdPocketActionFinder.Object);
	else
		UE_LOG(
		LogSingularisInventory,
		Error,
		TEXT("默认插槽 2 输入动作加载失败：%s"),
		TEXT("/SingularisInventory/Inputs/Actions/IA_ThirdPocket")
	);

	if (FourthPocketActionFinder.Succeeded())
		SelectSlotActions.Add(FourthPocketActionFinder.Object);
	else
		UE_LOG(
		LogSingularisInventory,
		Error,
		TEXT("默认插槽 3 输入动作加载失败：%s"),
		TEXT("/SingularisInventory/Inputs/Actions/IA_FourthPocket")
	);
}

void USingularisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	checkf(
		GetOwner()->IsA<APlayerController>(),
		TEXT("[%s] Owner 非 PlayerController（实际：%s）"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(GetOwner()->GetClass())
	);

	OwnerPlayerController = Cast<APlayerController>(GetOwner());

	BindInputAction();

	if (OwnerPlayerController.IsValid() && OwnerPlayerController->IsLocalController())
	{
		OwnerPlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessPawnChanged);
		RefreshInputMappingContext();

		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] BeginPlay：本地控制器初始化完成"),
			*GetNameSafe(GetOwner())
		);
	}
}

void USingularisInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerPlayerController.IsValid() && OwnerPlayerController->IsLocalController())
	{
		OwnerPlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPossessPawnChanged);

		if (IsValid(InputMappingContext))
		{
			if (const ULocalPlayer* LocalPlayer = OwnerPlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
					Subsystem->RemoveMappingContext(InputMappingContext);
			}
		}

		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] EndPlay：输入绑定与映射上下文已清理"),
			*GetNameSafe(GetOwner())
		);
	}

	Super::EndPlay(EndPlayReason);
}

AActor* USingularisInventoryComponent::SpawnItemInWorld(USingularisItem* Item, const FTransform Transform) const
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：物品实例无效"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 经全局查询子系统取物品形态 Actor 类
	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：GameInstance 无效"),
			*GetNameSafe(GetOwner())
		);
		return nullptr;
	}
	const USingularisInventoryItemSubsystem* ItemSubsystem = GameInstance->GetSubsystem<
		USingularisInventoryItemSubsystem>();
	if (!IsValid(ItemSubsystem))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品查询子系统无效"),
			*GetNameSafe(GetOwner())
		);
		return nullptr;
	}
	const TSubclassOf<AActor> FormActorClass = ItemSubsystem->GetFormActorClass(Item);
	if (!IsValid(FormActorClass))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：物品 %s 未配置形态 Actor 类"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item)
		);
		return nullptr;
	}

	// 3) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = GetWorld()->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SpawnItemInWorld：形态 Actor %s 生成失败"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(FormActorClass)
		);
		return nullptr;
	}

	// 4) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
		ItemComponent->BindItem(Item);
	else
		UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] SpawnItemInWorld：形态 Actor %s 无 ItemComponent，物品 %s 仅入世不可收容"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(FormActor),
		*GetNameSafe(Item)
	);

	// 5) 开启物理
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(FormActor->GetRootComponent()))
	{
		PrimitiveComponent->SetMobility(EComponentMobility::Movable);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PrimitiveComponent->SetSimulatePhysics(true);

		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] SpawnItemInWorld：形态 Actor %s 开启物理"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(FormActor)
		);
	}

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] SpawnItemInWorld：物品 %s(%s) 生成入世界成功"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
	return FormActor;
}

USingularisItem* USingularisInventoryComponent::CollectItem(AActor* FormActor) const
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] CollectItem：形态 Actor 无效"),
			*GetNameSafe(GetOwner())
		);
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
			*GetNameSafe(GetOwner()),
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
			*GetNameSafe(GetOwner()),
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
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
	return Item;
}

USingularisItem* USingularisInventoryComponent::PickupItem(AActor* FormActor)
{
	// 1) 收容出世界（TakeItem + Destroy），未指定容器，返回实例
	USingularisItem* Item = CollectItem(FormActor);
	if (Item == nullptr) return nullptr; // CollectItem 已记录日志

	// 2) 按规则路由入口袋：选中插槽为空则优放入选中插槽，否则寻找首个空插槽
	USingularisPocketComponent* Pocket = GetPocketComponent();
	if (IsValid(Pocket))
	{
		const int32 SelectedIndex = Pocket->GetSelectedIndex();
		bool bPlaced = SelectedIndex != INDEX_NONE
			&& Pocket->GetItem(SelectedIndex) == nullptr
			&& Pocket->AddItemAt(Item, SelectedIndex);

		if (!bPlaced)
			bPlaced = Pocket->AddItem(Item) != INDEX_NONE;

		if (bPlaced)
		{
			UE_LOG(
				LogSingularisInventory,
				Display,
				TEXT("[%s] PickupItem：物品 %s(%s) 已路由入口袋"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(Item),
				*GetNameSafe(Item->GetClass())
			);
			return Item;
		}
	}

	// 3) 未入容器：返回实例由调用方处置
	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] PickupItem：物品 %s(%s) 未入口袋，交由调用方处置"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
	return Item;
}

void USingularisInventoryComponent::DropItem(USingularisItem* Item)
{
	// 1) 零信任校验：物品实例与所控 Character 必须有效
	if (!IsValid(Item))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] DropItem：物品实例无效"), *GetNameSafe(GetOwner()));
		return;
	}
	USingularisPocketComponent* Pocket = GetPocketComponent();
	const ACharacter* Character = GetControlledCharacter();
	if (!IsValid(Pocket) || !IsValid(Character))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] DropItem：口袋或所控 Character 无效（口袋：%s，Character：%s）"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Pocket),
			*GetNameSafe(Character)
		);
		return;
	}

	// 2) 从口袋移除指定物品（relinquish 持有），避免与形态 Actor 双重持有
	if (!Pocket->RemoveItem(Item))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] DropItem：物品 %s 不在口袋中，无法丢弃"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Item)
		);
		return;
	}

	// 3) 生成入世界至角色前方（绑定到形态 Actor 的 ItemComponent）
	SpawnItemInWorld(Item, ComputeDropTransform(Character));

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] DropItem：物品 %s(%s) 已丢弃入世界"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Item),
		*GetNameSafe(Item->GetClass())
	);
}

void USingularisInventoryComponent::DropHeldItem()
{
	// 1) 仅本地控制器端：选中为本地行为
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] DropHeldItem：非本地控制者，跳过"),
			*GetNameSafe(GetOwner())
		);
		return;
	}
	const USingularisPocketComponent* Pocket = GetPocketComponent();
	if (!IsValid(Pocket))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] DropHeldItem：未找到口袋组件"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 2) 读本地手持物品 → 经 RPC 上行服务端执行丢弃
	USingularisItem* HeldItem = Pocket->GetSelectedItem();
	if (!IsValid(HeldItem))
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] DropHeldItem：无手持物品"),
			*GetNameSafe(GetOwner())
		);
		return;
	}
	Server_DropItem(HeldItem);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] DropHeldItem：手持物品 %s(%s) 已请求丢弃"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(HeldItem),
		*GetNameSafe(HeldItem->GetClass())
	);
}

void USingularisInventoryComponent::Server_DropItem_Implementation(USingularisItem* Item)
{
	DropItem(Item);
}

void USingularisInventoryComponent::BindInputAction()
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(
		OwnerPlayerController->InputComponent
	);
	if (!IsValid(EnhancedInputComponent))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] BindInputAction：EnhancedInputComponent 无效"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 1) 选中插槽：数组索引即插槽号
	for (auto i = 0; i < SelectSlotActions.Num(); ++i)
	{
		if (!IsValid(SelectSlotActions[i]))
			continue;

		EnhancedInputComponent->BindAction(
			SelectSlotActions[i],
			ETriggerEvent::Started,
			this,
			&USingularisInventoryComponent::HandleSelectSlot,
			i
		);
	}

	// 2) 丢弃
	if (IsValid(DropInputAction))
	{
		EnhancedInputComponent->BindAction(
			DropInputAction,
			ETriggerEvent::Started,
			this,
			&USingularisInventoryComponent::HandleDropInputAction
		);
	}
	else
		UE_LOG(
		LogSingularisInventory,
		Warning,
		TEXT("[%s] BindInputAction：丢弃输入动作未配置，丢弃功能不可用"),
		*GetNameSafe(GetOwner())
	);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] BindInputAction：绑定完成（选中动作 %d 个，丢弃 %s）"),
		*GetNameSafe(GetOwner()),
		SelectSlotActions.Num(),
		IsValid(DropInputAction) ? TEXT("已绑定") : TEXT("未绑定")
	);
}

void USingularisInventoryComponent::RefreshInputMappingContext() const
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;
	if (!IsValid(InputMappingContext))
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		OwnerPlayerController->GetLocalPlayer()
	);
	if (!IsValid(Subsystem))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] RefreshInputMappingContext：EnhancedInput 本地玩家子系统无效"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	if (GetControlledCharacter() != nullptr)
		Subsystem->AddMappingContext(InputMappingContext, InputPriority);
	else
		Subsystem->RemoveMappingContext(InputMappingContext);
}

ACharacter* USingularisInventoryComponent::GetControlledCharacter() const
{
	return OwnerPlayerController.IsValid() ? OwnerPlayerController->GetCharacter() : nullptr;
}

USingularisPocketComponent* USingularisInventoryComponent::GetPocketComponent() const
{
	const ACharacter* Character = GetControlledCharacter();
	if (!IsValid(Character))
		return nullptr;

	return Character->FindComponentByClass<USingularisPocketComponent>();
}

FTransform USingularisInventoryComponent::ComputeDropTransform(const ACharacter* Character) const
{
	if (!IsValid(Character))
		return FTransform::Identity;

	const FVector DropLocation =
		Character->GetActorLocation() +
		Character->GetActorForwardVector() * DropDistance +
		FVector(0.0f, 0.0f, DropZOffset);

	return FTransform(DropLocation);
}

// ReSharper disable CppMemberFunctionMayBeConst

void USingularisInventoryComponent::HandleSelectSlot(const FInputActionValue& Value, const int32 SlotIndex)
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;

	USingularisPocketComponent* Pocket = GetPocketComponent();
	if (!IsValid(Pocket))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] HandleSelectSlot：未找到口袋组件"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 选中为本地行为：客户端直接改本地 SelectedSlotIndex，不经服务端
	Pocket->SelectSlot(SlotIndex);
}

// ReSharper restore CppMemberFunctionMayBeConst

void USingularisInventoryComponent::HandleDropInputAction(const FInputActionValue& Value)
{
	// 丢弃手持：读本地手持 → RPC 上行服务端（逻辑封装于 DropHeldItem）
	DropHeldItem();
}

void USingularisInventoryComponent::OnPossessPawnChanged(APawn* OldPawn, APawn* NewPawn) const
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;

	// 所控 Character 变更（重生 / Possess 切换），刷新输入映射上下文增删
	RefreshInputMappingContext();
}
