#include "Components/SingularisPocketWidgetComponent.h"

#include <Blueprint/UserWidget.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <UObject/ConstructorHelpers.h>

#include "SingularisInventory.h"
#include "Components/SingularisPocketComponent.h"
#include "Interfaces/SingularisPocketViewInterface.h"
#include "Objects/SingularisItem.h"

USingularisPocketWidgetComponent::USingularisPocketWidgetComponent()
{
	SetIsReplicatedByDefault(false);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;

	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(
		TEXT(
			"/SingularisInventory/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget.WBP_SingularisInventory_SingularisPocketWidget_C"
		)
	);

	if (WidgetClassFinder.Succeeded())
		PocketWidgetClass = WidgetClassFinder.Class;
}

void USingularisPocketWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayerController = ResolveOwningLocalPlayerController();

	// 1) 非本地控制器端无需驱动视图，静默跳过
	if (!OwnerPlayerController.IsValid())
		return;

	// 2) 自动创建视图
	if (bAutoCreateView)
		CreatePocketView();

	// 3) 绑定口袋组件并推送一次全量状态
	ObservePocketComponent();
}

void USingularisPocketWidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1) 自动创建路径：屏幕空间控件不随组件销毁自动移除，需显式从视口移除避免残留
	if (bAutoCreateView)
	{
		if (UUserWidget* const PocketUserWidget = Cast<UUserWidget>(PocketView.GetObject()))
		{
			PocketUserWidget->RemoveFromParent();

			UE_LOG(
				LogSingularisInventory,
				Display,
				TEXT("[%s] EndPlay：口袋视图 %s 已从视口移除"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(PocketUserWidget)
			);
		}
	}

	// 2) 清空引用，事件绑定随组件销毁自动失效
	PocketView = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USingularisPocketWidgetComponent::SetPocketView(
	const TScriptInterface<ISingularisPocketViewInterface>& NewPocketView
)
{
	// 1) 自动创建视图时视图由组件自身托管，拒绝外部注入
	if (bAutoCreateView)
		return;

	// 2) 幂等检查，视图未变更时无需重复推送
	if (PocketView == NewPocketView)
		return;
	PocketView = NewPocketView;

	// 3) 非本地控制器端无视图宿主，无法推送
	if (!OwnerPlayerController.IsValid())
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SetPocketView：非本地控制器，无法推送视图状态"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 4) 外部注入后主动拉取一次全量状态，消除错过事件导致的空白期
	FullPull(ResolvePocketComponent());
}

void USingularisPocketWidgetComponent::HandleItemAdded(const int32 SlotIndex, USingularisItem* Item) const
{
	if (!IsValid(PocketView.GetObject()))
		return;

	ISingularisPocketViewInterface::Execute_OnItemAdded(PocketView.GetObject(), SlotIndex, Item);
}

void USingularisPocketWidgetComponent::HandleItemRemoved(const int32 SlotIndex, USingularisItem* Item) const
{
	if (!IsValid(PocketView.GetObject()))
		return;

	ISingularisPocketViewInterface::Execute_OnItemRemoved(PocketView.GetObject(), SlotIndex, Item);
}

void USingularisPocketWidgetComponent::HandleSelectionChanged(const int32 OldSlotIndex, const int32 NewSlotIndex) const
{
	if (!IsValid(PocketView.GetObject()))
		return;

	ISingularisPocketViewInterface::Execute_OnSelectionChanged(PocketView.GetObject(), OldSlotIndex, NewSlotIndex);
}

APlayerController* USingularisPocketWidgetComponent::ResolveOwningLocalPlayerController() const
{
	// 1) Owner 为 Pawn 时取其控制器，Owner 为 Controller 时直接使用
	const APawn* const OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* const PlayerController = IsValid(OwnerPawn)
		                                            ? Cast<APlayerController>(OwnerPawn->GetController())
		                                            : Cast<APlayerController>(GetOwner());

	// 2) 仅本客户端拥有的本地控制器才有效，避免为其他玩家复制的 Pawn 创建幽灵控件
	return IsValid(PlayerController) && PlayerController->IsLocalController() ? PlayerController : nullptr;
}

void USingularisPocketWidgetComponent::CreatePocketView()
{
	// 1) 本地玩家检查
	if (!OwnerPlayerController.IsValid())
		return;

	// 2) 创建口袋控件
	UUserWidget* const CreatedWidget = CreateWidget<UUserWidget>(OwnerPlayerController.Get(), PocketWidgetClass);
	if (!IsValid(CreatedWidget))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] CreatePocketView：控件类 %s 创建控件失败"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(PocketWidgetClass.Get())
		);
		return;
	}

	// MustImplement 仅约束编辑器选择器，C++ 与蓝图图赋值可绕过，创建后运行时复核接口实现
	if (!ensureMsgf(
		CreatedWidget->Implements<USingularisPocketViewInterface>(),
		TEXT("[%s] CreatePocketView：控件类 %s 未实现 SingularisPocketViewInterface"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PocketWidgetClass.Get())
	))
		return;

	// 3) 缓存视图并添加到视口
	PocketView = CreatedWidget;
	CreatedWidget->AddToViewport();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] CreatePocketView：口袋视图 %s 创建成功"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(CreatedWidget)
	);
}

USingularisPocketComponent* USingularisPocketWidgetComponent::ResolvePocketComponent() const
{
	// 1) 本地 PlayerController 未就绪时无法经 PlayerController -> Character 链查找
	if (!OwnerPlayerController.IsValid())
		return nullptr;

	// 2) 口袋组件挂在所控 Pawn（Character）上，经控制器链路获取
	const APawn* const OwnerPawn = OwnerPlayerController->GetPawn();
	return IsValid(OwnerPawn) ? OwnerPawn->FindComponentByClass<USingularisPocketComponent>() : nullptr;
}

void USingularisPocketWidgetComponent::ObservePocketComponent()
{
	// 1) 本地玩家检查
	if (!OwnerPlayerController.IsValid())
		return;

	// 2) 获取同属主的口袋组件
	USingularisPocketComponent* const PocketComponent = ResolvePocketComponent();
	if (!IsValid(PocketComponent))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] ObservePocketComponent：未在所控 Pawn 上找到口袋组件"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 3) 绑定口袋事件
	PocketComponent->OnItemAddedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemAdded);
	PocketComponent->OnItemRemovedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemRemoved);
	PocketComponent->OnSelectionChangedEvent.AddDynamic(
		this,
		&USingularisPocketWidgetComponent::HandleSelectionChanged
	);

	// 4) 绑定后主动拉取一次全量状态，消除错过事件导致的空白期
	FullPull(PocketComponent);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] ObservePocketComponent：已绑定口袋组件 %s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PocketComponent)
	);
}

void USingularisPocketWidgetComponent::FullPull(const USingularisPocketComponent* PocketComponent) const
{
	// 1) 零信任校验：视图与口袋组件必须有效
	if (!IsValid(PocketView.GetObject()) || !IsValid(PocketComponent))
		return;

	// 2) 聚合各插槽物品
	const int32 Capacity = PocketComponent->Capacity;
	TArray<USingularisItem*> Items;
	Items.Reserve(Capacity);
	for (auto Index = 0; Index < Capacity; ++Index)
		Items.Add(PocketComponent->GetItem(Index));

	// 3) 经 SPI 推送全量状态
	ISingularisPocketViewInterface::Execute_OnPocketRefresh(
		PocketView.GetObject(),
		Capacity,
		Items,
		PocketComponent->GetSelectedIndex()
	);
}
