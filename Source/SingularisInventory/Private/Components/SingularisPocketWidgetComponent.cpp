#include "Components/SingularisPocketWidgetComponent.h"

#include <UMG.h>
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
	else
	{
		UE_LOG(
			LogSingularisInventory,
			Error,
			TEXT("默认口袋视图加载失败：%s"),
			TEXT("/SingularisInventory/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget")
		);
	}
}

void USingularisPocketWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1) AutoCreate 路径：创建视图并加入视口；External 路径视图由用户提供
	if (PocketViewMode == ESingularisPocketViewMode::AutoCreate)
		CreatePocketView();

	// 2) 幂等观察入口：双就绪（视图 + 口袋）时绑定事件并全量拉取
	TryStartObservation();
}

void USingularisPocketWidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1) AutoCreate 路径：屏幕空间控件不随组件销毁自动移除，需显式从视口移除避免残留；
	//    External 路径：视图为用户资产，不做任何生命周期操作
	if (PocketViewMode == ESingularisPocketViewMode::AutoCreate)
	{
		if (UUserWidget* PocketUserWidget = Cast<UUserWidget>(PocketView.GetObject()))
			PocketUserWidget->RemoveFromParent();
	}

	// 2) 两模式统一清空引用，事件绑定随组件销毁自动失效
	PocketView = nullptr;
	ResolvedPocketComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void USingularisPocketWidgetComponent::SetPocketView(const TScriptInterface<ISingularisPocketViewInterface>& NewPocketView)
{
	// 1) 契约显式：外部注入仅 External 模式可用，避免与自动创建路径冲突
	if (PocketViewMode != ESingularisPocketViewMode::External)
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] SetPocketView：当前视图模式非外部注入，忽略本次设置"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 2) 替换视图：新视图立即收到一次全量刷新，旧视图自然停止接收事件；传空表示停止驱动
	PocketView = NewPocketView;

	if (bBound)
	{
		if (IsValid(PocketView.GetObject()) && IsValid(ResolvedPocketComponent.Get()))
			RefreshPocket(ResolvedPocketComponent.Get());
	}
	else
		TryStartObservation();
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
	{
		UE_LOG(
			LogSingularisInventory,
			Display,
			TEXT("[%s] ResolveOwningLocalPlayerController：非本地控制者，跳过 UI"),
			*GetNameSafe(GetOwner())
		);
		return nullptr;
	}

	return PlayerController;
}

void USingularisPocketWidgetComponent::CreatePocketView()
{
	// 1) 仅本客户端拥有的 Actor（Pawn 由本地 PC 控制，或 Owner 本身即本地 PC）才创建 UI，
	//    避免为其他玩家复制的 Pawn 创建幽灵控件
	APlayerController* PlayerController = ResolveOwningLocalPlayerController();
	if (!IsValid(PlayerController))
		return; // 非本地控制者，ResolveOwningLocalPlayerController 已记录 Display

	// 2) 零信任校验：未配置视图类则跳过
	if (!IsValid(PocketWidgetClass))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] CreatePocketView：未配置视图类"), *GetNameSafe(GetOwner()));
		return;
	}

	UUserWidget* CreatedWidget = CreateWidget<UUserWidget>(PlayerController, PocketWidgetClass);
	if (!ensureMsgf(
		IsValid(CreatedWidget),
		TEXT("[%s] CreatePocketView：创建视图 %s 失败"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PocketWidgetClass.Get())
	))
		return;

	// 3) 零信任校验：MustImplement 仅约束编辑器选择器，C++ / 蓝图图赋值可绕过，运行时复核接口实现
	if (!ensureMsgf(
		CreatedWidget->ImplementsInterface(USingularisPocketViewInterface::StaticClass()),
		TEXT("[%s] CreatePocketView：视图类 %s 未实现 SingularisPocketViewInterface"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PocketWidgetClass.Get())
	))
		return;

	// 4) 写入视图并加入视口；TScriptInterface 赋值自动计算接口指针
	PocketView = CreatedWidget;
	CreatedWidget->AddToViewport();

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] CreatePocketView：视图 %s 创建成功并加入视口"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PocketWidgetClass.Get())
	);
}

USingularisPocketComponent* USingularisPocketWidgetComponent::ResolvePocketComponent() const
{
	return Cast<USingularisPocketComponent>(PocketComponentReference.GetComponent(GetOwner()));
}

void USingularisPocketWidgetComponent::TryStartObservation()
{
	// 1) 幂等守卫：已绑定则跳过
	if (bBound)
		return;

	// 2) 视图未就绪属预期状态（External 模式视图常晚于本组件 BeginPlay），静默等待
	if (!IsValid(PocketView.GetObject()))
		return;

	// 3) 门控：仅本客户端拥有的 Actor 才驱动 UI
	if (!IsValid(ResolveOwningLocalPlayerController()))
		return; // 非本地控制者，ResolveOwningLocalPlayerController 已记录 Display

	// 4) 零信任校验：引用指向同 Actor 组件，BeginPlay 后必已存在，解析失败即配置错误
	USingularisPocketComponent* PocketComponent = ResolvePocketComponent();
	if (!IsValid(PocketComponent))
	{
		UE_LOG(
			LogSingularisInventory,
			Warning,
			TEXT("[%s] TryStartObservation：未解析到口袋组件，请检查 PocketComponentReference 配置"),
			*GetNameSafe(GetOwner())
		);
		return;
	}

	// 5) 绑定事件实现事件驱动观察者模式
	PocketComponent->OnItemAddedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemAdded);
	PocketComponent->OnItemRemovedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemRemoved);
	PocketComponent->OnSelectionChangedEvent.AddDynamic(
		this,
		&USingularisPocketWidgetComponent::HandleSelectionChanged
	);
	ResolvedPocketComponent = PocketComponent;
	bBound = true;

	// 6) 主动拉取一次全量状态，消除错过事件导致的空白期
	RefreshPocket(PocketComponent);

	UE_LOG(
		LogSingularisInventory,
		Display,
		TEXT("[%s] TryStartObservation：已绑定 %s 事件并完成全量拉取"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PocketComponent)
	);
}

void USingularisPocketWidgetComponent::RefreshPocket(const USingularisPocketComponent* PocketComponent) const
{
	if (!IsValid(PocketView.GetObject()) || !IsValid(PocketComponent))
		return;

	// 1) 聚合各插槽物品
	const int32 Capacity = PocketComponent->Capacity;
	TArray<USingularisItem*> Items;
	Items.Reserve(Capacity);
	for (auto i = 0; i < Capacity; ++i)
		Items.Add(PocketComponent->GetItem(i));

	// 2) 经 SPI 推送全量状态
	ISingularisPocketViewInterface::Execute_OnPocketRefresh(
		PocketView.GetObject(),
		Capacity,
		Items,
		PocketComponent->GetSelectedIndex()
	);
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
