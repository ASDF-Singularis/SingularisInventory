#include "Components/SingularisPocketWidgetComponent.h"

#include <UMG.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>

#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "Widgets/SingularisPocketWidget.h"

USingularisPocketWidgetComponent::USingularisPocketWidgetComponent()
{
	SetIsReplicatedByDefault(false);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisPocketWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	CreatePocketWidget();
	ObservePocketComponent();
}

void USingularisPocketWidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1) 屏幕空间控件不随组件销毁自动移除，需显式从视口移除避免残留
	if (IsValid(PocketWidget))
	{
		PocketWidget->RemoveFromParent();
		PocketWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

APlayerController* USingularisPocketWidgetComponent::ResolveOwningLocalPlayerController() const
{
	// 1) Owner 为 Pawn 时，取其控制器；Owner 为 Controller 时直接使用
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController;
	if (IsValid(OwnerPawn))
		PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	else
		PlayerController = Cast<APlayerController>(GetOwner());

	// 2) 仅本客户端拥有的本地控制器才有效，避免为其他玩家复制的 Pawn 创建幽灵控件
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		return nullptr;

	return PlayerController;
}

void USingularisPocketWidgetComponent::CreatePocketWidget()
{
	// 1) 仅本客户端拥有的 Actor（Pawn 由本地 PC 控制，或 Owner 本身即本地 PC）才创建 UI，
	//    避免为其他玩家复制的 Pawn 创建幽灵控件
	APlayerController* PlayerController = ResolveOwningLocalPlayerController();
	if (!IsValid(PlayerController))
		return;

	// 2) 零信任校验：未配置控件类则跳过
	if (!IsValid(PocketWidgetClass))
		return;

	PocketWidget = CreateWidget<USingularisPocketWidget>(PlayerController, PocketWidgetClass);
	if (!IsValid(PocketWidget))
		return;

	PocketWidget->AddToViewport();
}

void USingularisPocketWidgetComponent::ObservePocketComponent()
{
	const APlayerController* PlayerController = ResolveOwningLocalPlayerController();
	if (!IsValid(PlayerController))
		return;

	// 1) 零信任校验：控件与目标组件均需有效
	if (!IsValid(PocketWidget))
		return;

	USingularisPocketComponent* PocketComponent = Cast<USingularisPocketComponent>(
		PocketComponentReference.GetComponent(GetOwner())
	);
	if (!IsValid(PocketComponent))
		return;

	// 2) 绑定事件实现事件驱动观察者模式
	PocketComponent->OnItemAddedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemAdded);
	PocketComponent->OnItemRemovedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemRemoved);
	PocketComponent->OnSelectionChangedEvent.AddDynamic(
		this,
		&USingularisPocketWidgetComponent::HandleSelectionChanged
	);

	// 3) 主动拉取一次全量状态，消除错过事件导致的空白期
	RefreshPocket(PocketComponent);
}

void USingularisPocketWidgetComponent::RefreshPocket(const USingularisPocketComponent* PocketComponent) const
{
	if (!IsValid(PocketWidget) || !IsValid(PocketComponent))
		return;

	// 1) 聚合各插槽物品
	const int32 Capacity = PocketComponent->Capacity;
	TArray<USingularisItem*> Items;
	Items.Reserve(Capacity);
	for (auto i = 0; i < Capacity; ++i)
		Items.Add(PocketComponent->GetItem(i));

	// 2) 经 SPI 推送全量状态
	PocketWidget->OnPocketRefresh(Capacity, Items, PocketComponent->GetSelectedIndex());
}

void USingularisPocketWidgetComponent::HandleItemAdded(const int32 SlotIndex, USingularisItem* Item) const
{
	if (!IsValid(PocketWidget))
		return;

	PocketWidget->OnItemAdded(SlotIndex, Item);
}

void USingularisPocketWidgetComponent::HandleItemRemoved(const int32 SlotIndex, USingularisItem* Item) const
{
	if (!IsValid(PocketWidget))
		return;

	PocketWidget->OnItemRemoved(SlotIndex, Item);
}

void USingularisPocketWidgetComponent::HandleSelectionChanged(const int32 OldSlotIndex, const int32 NewSlotIndex) const
{
	if (!IsValid(PocketWidget))
		return;

	PocketWidget->OnSelectionChanged(OldSlotIndex, NewSlotIndex);
}
