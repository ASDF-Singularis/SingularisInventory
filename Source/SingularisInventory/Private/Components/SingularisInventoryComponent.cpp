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
}

void USingularisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	checkf(
		GetOwner()->IsA<APlayerController>(),
		TEXT("SingularisInventoryComponent: Owner not is PlayerController")
	);

	OwnerPlayerController = Cast<APlayerController>(GetOwner());

	BindInputAction();

	if (OwnerPlayerController.IsValid() && OwnerPlayerController->IsLocalController())
	{
		OwnerPlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessPawnChanged);
		RefreshInputMappingContext();
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
	}

	Super::EndPlay(EndPlayReason);
}

AActor* USingularisInventoryComponent::SpawnItemInWorld(USingularisItem* Item, FTransform Transform)
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
		return nullptr;

	// 2) 经全局查询子系统取物品形态 Actor 类
	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!IsValid(GameInstance))
		return nullptr;
	const USingularisInventoryItemSubsystem* ItemSubsystem = GameInstance->GetSubsystem<
		USingularisInventoryItemSubsystem>();
	if (!IsValid(ItemSubsystem))
		return nullptr;
	const TSubclassOf<AActor> FormActorClass = ItemSubsystem->GetFormActorClass(Item);
	if (!IsValid(FormActorClass))
		return nullptr;

	// 3) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = GetWorld()->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!IsValid(FormActor))
		return nullptr;

	// 4) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
		ItemComponent->BindItem(Item);

	return FormActor;
}

USingularisItem* USingularisInventoryComponent::CollectItem(
	AActor* FormActor,
	USingularisPocketComponent* TargetContainer
)
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
		return nullptr;

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
		return nullptr;

	// 3) 取回物品实例；无物品则不销毁形态 Actor
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
		return nullptr;

	// 4) 销毁形态 Actor
	FormActor->Destroy();

	// 5) 提供目标容器则尝试入容器；满或未提供容器时返回实例由调用方处置
	if (IsValid(TargetContainer))
		TargetContainer->AddItem(Item);

	return Item;
}

USingularisItem* USingularisInventoryComponent::PickupItem(AActor* FormActor)
{
	// 1) 收容出世界（TakeItem + Destroy），未指定容器，返回实例
	USingularisItem* Item = CollectItem(FormActor);
	if (Item == nullptr)
		return nullptr;

	// 2) 按规则路由：口袋优先（满则返回实例，未来扩展背包兜底）
	USingularisPocketComponent* Pocket = GetPocketComponent();
	if (IsValid(Pocket) && Pocket->AddItem(Item) != INDEX_NONE)
		return Item;

	// 3) 未入容器：返回实例由调用方处置
	return Item;
}

void USingularisInventoryComponent::DropSelectedItem(const int32 SlotIndex)
{
	// 1) 取口袋与所控 Character
	USingularisPocketComponent* Pocket = GetPocketComponent();
	const ACharacter* Character = GetControlledCharacter();
	if (!IsValid(Pocket) || !IsValid(Character))
		return;

	// 2) 从口袋取出物品（relinquish 持有），避免与形态 Actor 双重持有
	USingularisItem* Item = Pocket->RemoveItemAt(SlotIndex);
	if (Item == nullptr)
		return;

	// 3) 生成入世界至角色前方（绑定到形态 Actor 的 ItemComponent）
	SpawnItemInWorld(Item, ComputeDropTransform(Character));
}

void USingularisInventoryComponent::Server_DropSelectedItem_Implementation(const int32 SlotIndex)
{
	DropSelectedItem(SlotIndex);
}

void USingularisInventoryComponent::BindInputAction()
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(
		OwnerPlayerController->InputComponent
	);
	if (!IsValid(EnhancedInputComponent))
		return;

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
		return;

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
		return;

	// 选中为本地行为：客户端直接改本地 SelectedSlotIndex，不经服务端
	Pocket->SelectSlot(SlotIndex);
}

// ReSharper restore CppMemberFunctionMayBeConst

void USingularisInventoryComponent::HandleDropInputAction(const FInputActionValue& Value)
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;

	const USingularisPocketComponent* Pocket = GetPocketComponent();
	if (!IsValid(Pocket))
		return;

	const int32 SlotIndex = Pocket->GetSelectedIndex();
	if (SlotIndex == INDEX_NONE)
		return;

	// 丢弃需服务端执行（SpawnActor 服务端权威），经 RPC 上行
	Server_DropSelectedItem(SlotIndex);
}

void USingularisInventoryComponent::OnPossessPawnChanged(APawn* OldPawn, APawn* NewPawn) const
{
	if (!OwnerPlayerController.IsValid() || !OwnerPlayerController->IsLocalController())
		return;

	// 所控 Character 变更（重生 / Possess 切换），刷新输入映射上下文增删
	RefreshInputMappingContext();
}
