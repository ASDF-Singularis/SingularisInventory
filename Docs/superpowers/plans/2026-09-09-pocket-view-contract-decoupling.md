# 引力奇点口袋视图契约解耦实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `USingularisPocketWidgetComponent` 对 `USingularisPocketWidget` 的硬依赖解耦为 `ISingularisPocketViewInterface` 视图契约，并提供 AutoCreate / External 双路径实例化。

**Architecture:** 视图契约（4 个 SPI 函数）从具体 Widget 基类提取为 UInterface；组件内部以 `TScriptInterface` 缓存统一驱动，`ESingularisPocketViewMode` 枚举显式选择自动创建（组件拥有生命周期）或外部注入（仅驱动不拥有）；`TryStartObservation` 幂等入口处理双路径时序。

**Tech Stack:** Unreal Engine 5.8, C++, UMG, UInterface, TScriptInterface

**Spec:** `Docs/superpowers/specs/2026-09-09-pocket-view-contract-decoupling-design.md`（同仓库）

## Global Constraints

- **禁止编译验证与 IDE 诊断**：本 IDE 未配置 LSP，任何任务不得触发编译或使用 Project Diagnostics；每任务以"静态自查"清单代替测试运行
- 遵循 Singularis Skeleton：`#pragma region` 排序、DisplayName 中文、文档注释简洁客观、函数体内部序号注释（`1) 2) 3)`）
- 模块 API 宏 `SINGULARISINVENTORY_API`；类别前缀 `SingularisInventory|引力奇点口袋*`
- 接口事件覆盖为最小同构（仅 4 函数：全量刷新 + 加入 + 移除 + 选中变化），不得擅自扩展
- `SingularisInventoryEditor` 模块零变更
- 本插件为独立 git 仓库，仓库根 `D:/UnrealProjects/VehicleTour/Plugins/SingularisInventory`；所有路径相对该仓库根
- git 提交在插件仓库根执行，不创建分支

## File Structure

| 文件 | 操作 | 职责 |
|------|------|------|
| `Source/SingularisInventory/Public/Interfaces/SingularisPocketViewInterface.h` | 新建 | 视图契约接口（4 SPI 函数） |
| `Source/SingularisInventory/Private/Interfaces/SingularisPocketViewInterface.cpp` | 新建 | 仅含 include（同构 ItemFormActorInterface） |
| `Source/SingularisInventory/Public/Types/SingularisPocketType.h` | 修改 | 追加 `ESingularisPocketViewMode` 枚举 |
| `Source/SingularisInventory/Public/Widgets/SingularisPocketWidget.h` | 修改 | 去除 Abstract，改为实现接口 |
| `Source/SingularisInventory/Private/Widgets/SingularisPocketWidget.cpp` | 无变更 | 函数定义签名不变 |
| `Source/SingularisInventory/Public/Components/SingularisPocketWidgetComponent.h` | 修改 | 双路径属性 + API + 内部状态重排 |
| `Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp` | 修改 | 双路径生命周期与幂等观察 |
| `Content/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget` | 人工迁移 | 编辑器内重编译重保存 |

---

### Task 1: 视图契约接口

**Files:**
- Create: `Source/SingularisInventory/Public/Interfaces/SingularisPocketViewInterface.h`
- Create: `Source/SingularisInventory/Private/Interfaces/SingularisPocketViewInterface.cpp`

**Interfaces:**
- Produces: `ISingularisPocketViewInterface`，SPI 函数 `OnPocketRefresh(int32, const TArray<USingularisItem*>&, int32)` / `OnItemAdded(int32, USingularisItem*)` / `OnItemRemoved(int32, USingularisItem*)` / `OnSelectionChanged(int32, int32)`，均为 `BlueprintNativeEvent + BlueprintCallable`；静态调用形式 `ISingularisPocketViewInterface::Execute_<函数名>(UObject*, ...)`

- [ ] **Step 1: 提交 spec 文档**

```bash
git add Docs/superpowers/specs/2026-09-09-pocket-view-contract-decoupling-design.md
git commit -m "docs: 添加口袋视图契约解耦实施设计"
```

- [ ] **Step 2: 创建接口头文件**

文件内容（完整）：

```cpp
#pragma once

#include <CoreMinimal.h>
#include <UObject/Interface.h>

#include "SingularisPocketViewInterface.generated.h"

class USingularisItem;

/**
 * 引力奇点口袋视图接口。
 *
 * 口袋视图契约：实现者接收口袋组件的全量刷新与增量事件，由 WidgetComponent 在
 * 绑定完成与视图替换时经 Execute_ 调用。实现者不限于控件——任意 UObject 实现
 * 本接口即可接入驱动。
 */
UINTERFACE(Blueprintable, BlueprintType)
class USingularisPocketViewInterface : public UInterface
{
	GENERATED_BODY()
};

class SINGULARISINVENTORY_API ISingularisPocketViewInterface
{
	GENERATED_BODY()

public:
#pragma region SPI

	/**
	 * 口袋整体刷新：容量、各插槽物品、当前选中索引。
	 * 由 WidgetComponent 在绑定完成与视图替换时主动调用，消除错过事件导致的空白期；
	 * 外部注入的视图必须预期本函数在其被设置后立即到达。
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋视图接口|SPI",
		meta = (DisplayName = "口袋刷新")
	)
	void OnPocketRefresh(int32 Capacity, const TArray<USingularisItem*>& Items, int32 SelectedSlotIndex);

	/** 物品加入指定插槽。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋视图接口|SPI",
		meta = (DisplayName = "物品加入")
	)
	void OnItemAdded(int32 SlotIndex, USingularisItem* Item);

	/** 物品从指定插槽移除。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋视图接口|SPI",
		meta = (DisplayName = "物品移除")
	)
	void OnItemRemoved(int32 SlotIndex, USingularisItem* Item);

	/** 选中插槽变化。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋视图接口|SPI",
		meta = (DisplayName = "选中变化")
	)
	void OnSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex);

#pragma endregion
};
```

- [ ] **Step 3: 创建接口源文件**

文件内容（完整）：

```cpp
#include "Interfaces/SingularisPocketViewInterface.h"
```

- [ ] **Step 4: 静态自查**

- 4 个函数签名与原 `USingularisPocketWidget` SPI 逐字一致（仅类别不同）
- `SINGULARISINVENTORY_API` 仅在 `ISingularisPocketViewInterface` 类上（同构 `SingularisItemFormActorInterface.h`）
- `USingularisItem` 为前置声明，头文件未 include `Objects/SingularisItem.h`
- cpp 仅含一行 include

- [ ] **Step 5: 提交**

```bash
git add Source/SingularisInventory/Public/Interfaces/SingularisPocketViewInterface.h Source/SingularisInventory/Private/Interfaces/SingularisPocketViewInterface.cpp
git commit -m "feat(SingularisInventory): 新增引力奇点口袋视图接口"
```

---

### Task 2: 实例化模式枚举

**Files:**
- Modify: `Source/SingularisInventory/Public/Types/SingularisPocketType.h`

**Interfaces:**
- Produces: `ESingularisPocketViewMode : uint8`，取值 `AutoCreate` / `External`，`UENUM(BlueprintType)`

- [ ] **Step 1: 在 `ESingularisPocketOccupancy` 枚举之后、`FSingularisPocketSlot` 结构体之前插入以下内容**

```cpp
/**
 * 引力奇点口袋视图实例化模式
 */
UENUM(BlueprintType)
enum class ESingularisPocketViewMode : uint8
{
	/** 自动创建：按口袋视图类创建实例并加入视口，组件拥有其完整生命周期。 */
	AutoCreate UMETA(DisplayName = "自动创建"),

	/** 外部注入：用户经 SetPocketView 提供实现接口的实例，组件仅驱动、不拥有。 */
	External UMETA(DisplayName = "外部注入"),
};
```

- [ ] **Step 2: 静态自查**

- 插入位置正确（两个既有类型之间，不改动它们）
- 枚举值 UMETA DisplayName 与既有 `ESingularisPocketOccupancy` 的中文风格一致

- [ ] **Step 3: 提交**

```bash
git add Source/SingularisInventory/Public/Types/SingularisPocketType.h
git commit -m "feat(SingularisInventory): 新增口袋视图实例化模式枚举"
```

---

### Task 3: 默认视图实现接口

**Files:**
- Modify: `Source/SingularisInventory/Public/Widgets/SingularisPocketWidget.h`
- 无变更: `Source/SingularisInventory/Private/Widgets/SingularisPocketWidget.cpp`（函数定义签名不变，继承来源变化不影响定义体）

**Interfaces:**
- Consumes: `ISingularisPocketViewInterface`（Task 1）
- Produces: `USingularisPocketWidget : public UUserWidget, public ISingularisPocketViewInterface`，覆盖 4 个 `_Implementation`；`UCLASS(Blueprintable)`（去除 Abstract）

- [ ] **Step 1: 用以下内容覆盖头文件**

```cpp
#pragma once

#include <CoreMinimal.h>
#include <Blueprint/UserWidget.h>

#include "Interfaces/SingularisPocketViewInterface.h"
#include "Objects/SingularisItem.h"
#include "SingularisPocketWidget.generated.h"

/**
 * 引力奇点口袋控件。
 *
 * 默认口袋视图：实现 ISingularisPocketViewInterface，框架（USingularisPocketWidgetComponent）
 * 经接口推送口袋状态与数据。用户在蓝图或 C++ 子类中覆写 SPI，更新具体控件实现。
 */
UCLASS(Blueprintable)
class SINGULARISINVENTORY_API USingularisPocketWidget : public UUserWidget, public ISingularisPocketViewInterface
{
	GENERATED_BODY()

public:
#pragma region SPI

	/** 口袋整体刷新：容量、各插槽物品、当前选中索引。 */
	virtual void OnPocketRefresh_Implementation(
		int32 Capacity,
		const TArray<USingularisItem*>& Items,
		int32 SelectedSlotIndex
	) override;

	/** 物品加入指定插槽。 */
	virtual void OnItemAdded_Implementation(int32 SlotIndex, USingularisItem* Item) override;

	/** 物品从指定插槽移除。 */
	virtual void OnItemRemoved_Implementation(int32 SlotIndex, USingularisItem* Item) override;

	/** 选中插槽变化。 */
	virtual void OnSelectionChanged_Implementation(int32 OldSlotIndex, int32 NewSlotIndex) override;

#pragma endregion
};
```

- [ ] **Step 2: 确认 cpp 无需变更**

`SingularisPocketWidget.cpp` 中 4 个 `USingularisPocketWidget::On*_Implementation` 定义与新声明完全匹配，保持原样，不改动。

- [ ] **Step 3: 静态自查**

- `UCLASS(Blueprintable)`，无 `Abstract`
- 继承列表 `public UUserWidget, public ISingularisPocketViewInterface`
- 4 个 override 与接口签名一致（含 `const TArray<USingularisItem*>&`）
- cpp 文件内容与头文件声明匹配（打开确认，不修改）

- [ ] **Step 4: 提交**

```bash
git add Source/SingularisInventory/Public/Widgets/SingularisPocketWidget.h
git commit -m "refactor(SingularisInventory): 口袋控件改为实现视图接口并去除 Abstract"
```

---

### Task 4: WidgetComponent 双路径重构

**Files:**
- Modify: `Source/SingularisInventory/Public/Components/SingularisPocketWidgetComponent.h`（整文件覆盖）
- Modify: `Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp`（整文件覆盖）

**Interfaces:**
- Consumes: `ISingularisPocketViewInterface`（Task 1）、`ESingularisPocketViewMode`（Task 2）、`USingularisPocketComponent` 既有事件（`OnItemAddedEvent` / `OnItemRemovedEvent` / `OnSelectionChangedEvent`）
- Produces: `SetPocketView(const TScriptInterface<ISingularisPocketViewInterface>&)`（BlueprintCallable）、属性 `PocketView` / `PocketViewMode` / `PocketWidgetClass`（`TSubclassOf<UUserWidget>`）

- [ ] **Step 1: 用以下内容覆盖组件头文件**

```cpp
#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>
#include <UObject/ScriptInterface.h>

#include "Interfaces/SingularisPocketViewInterface.h"
#include "Types/SingularisPocketType.h"
#include "SingularisPocketWidgetComponent.generated.h"

class USingularisPocketComponent;
class APlayerController;
class UUserWidget;
struct FComponentReference;

/**
 * 引力奇点口袋控件组件。
 *
 * 屏幕空间 UI 观察者：通过 ComponentReference 配置观察目标 USingularisPocketComponent，
 * 初始化时解析目标、绑定其事件以实现事件驱动观察者模式，并主动拉取一次全量状态
 * 消除错过事件导致的空白期，随后将观察结果经 SPI 推送至口袋视图。
 *
 * 视图实例化双路径（ESingularisPocketViewMode）：
 * - AutoCreate：按 PocketWidgetClass 创建视图并加入视口，组件拥有其完整生命周期。
 * - External：用户经 SetPocketView 注入实现 ISingularisPocketViewInterface 的任意实例，
 *   组件仅驱动、不拥有——不创建、不挂载视口、不移除、不销毁。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点口袋控件组件")
)
class SINGULARISINVENTORY_API USingularisPocketWidgetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Instantiation

	UPROPERTY(
		Transient,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|Instantiation",
		meta = (DisplayName = "口袋视图")
	)
	TScriptInterface<ISingularisPocketViewInterface> PocketView{};

#pragma endregion

#pragma region Parameter

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|参数",
		meta = (DisplayName = "视图模式")
	)
	ESingularisPocketViewMode PocketViewMode = ESingularisPocketViewMode::AutoCreate;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|引用",
		meta = (
			DisplayName = "口袋组件引用",
			UseComponentPicker,
			AllowedClasses = "/Script/SingularisInventory.SingularisPocketComponent"
		)
	)
	FComponentReference PocketComponentReference{};

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|参数",
		meta = (
			DisplayName = "口袋视图类",
			MustImplement = "SingularisPocketViewInterface",
			EditCondition = "PocketViewMode == ESingularisPocketViewMode::AutoCreate"
		)
	)
	TSubclassOf<UUserWidget> PocketWidgetClass = nullptr;

#pragma endregion

#pragma region Constructors

	USingularisPocketWidgetComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma endregion

#pragma region API

	/**
	 * 设置外部口袋视图，仅 External 模式可用。
	 * 立即触发绑定与全量拉取；重复调用视为替换视图，新视图收到一次全量刷新。
	 * 传空表示停止驱动。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋控件|API",
		meta = (DisplayName = "设置口袋视图")
	)
	void SetPocketView(const TScriptInterface<ISingularisPocketViewInterface>& NewPocketView);

#pragma endregion

private:
#pragma region Internal Variable

	/** 口袋事件绑定去重守卫。 */
	bool bBound = false;

	/** 绑定时解析的口袋组件缓存，供视图替换后的补刷新使用。 */
	TWeakObjectPtr<USingularisPocketComponent> ResolvedPocketComponent = nullptr;

#pragma endregion

#pragma region Callback

	UFUNCTION()
	void HandleItemAdded(int32 SlotIndex, USingularisItem* Item) const;

	UFUNCTION()
	void HandleItemRemoved(int32 SlotIndex, USingularisItem* Item) const;

	UFUNCTION()
	void HandleSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex) const;

#pragma endregion

#pragma region Internal Function

	/** 解析本客户端拥有的本地 PlayerController，Owner 为 Pawn 或 Controller 时均适用。 */
	APlayerController* ResolveOwningLocalPlayerController() const;

	/** 按 PocketWidgetClass 创建视图并加入视口，仅 AutoCreate 模式调用。 */
	void CreatePocketView();

	/** 解析口袋组件引用，失败返回 nullptr。 */
	USingularisPocketComponent* ResolvePocketComponent() const;

	/** 幂等观察入口：双就绪（视图 + 口袋）时绑定事件并全量拉取。 */
	void TryStartObservation();

	/** 聚合口袋全量状态，经 SPI 推送至视图。 */
	void RefreshPocket(const USingularisPocketComponent* PocketComponent) const;

#pragma endregion
};
```

注意：`USingularisItem` 前置声明省略——`Types/SingularisPocketType.h` 已包含其完整定义。

- [ ] **Step 2: 用以下内容覆盖组件源文件**

```cpp
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
```

- [ ] **Step 3: 静态自查**

- `PocketWidget` 属性已删除，`PocketView` 为唯一视图句柄（单一数据源）
- `SetPocketView` 模式守卫 → 赋值 → 补刷新 / `TryStartObservation` 分支完整
- `TryStartObservation` 六个序号步骤齐全，`bBound` 在绑定前置位
- `EndPlay`：AutoCreate 才 `RemoveFromParent`，两模式统一清空引用
- `Execute_` 静态调用第一参数均为 `PocketView.GetObject()`
- 日志格式（`[Owner] 函数名：说明`）与原文件一致
- 头文件与源文件的函数声明/定义一一对应

- [ ] **Step 4: 提交**

```bash
git add Source/SingularisInventory/Public/Components/SingularisPocketWidgetComponent.h Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp
git commit -m "refactor(SingularisInventory): 口袋控件组件双路径实例化重构"
```

---

### Task 5: 编辑器迁移与人工验证（用户执行）

**Files:**
- 迁移: `Content/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget`

本任务无法由代理执行（需 Unreal Editor），执行到此处时停下，将以下清单交给用户。

- [ ] **Step 1: 代理暂停，向用户交付以下人工步骤**

1. 在编辑器中编译 SingularisInventory 模块，确认无编译错误。
2. 打开 `WBP_SingularisInventory_SingularisPocketWidget`，重编译并保存；确认 4 个事件（口袋刷新 / 物品加入 / 物品移除 / 选中变化）覆写仍然存在。若个别事件节点丢失绑定，删除后重新添加同名事件覆写（C++ 侧为空实现，真实逻辑全在 WBP 内，可对照事件名恢复）。
3. 路径 A 回归：默认配置（AutoCreate + 默认类）PIE，控件出现，物品加入 / 移除 / 选中切换正确反映。
4. 路径 B 基础：自建 HUD Widget 实现接口（类设置勾选 `SingularisPocketViewInterface`），组件设为 External，BeginPlay 末尾调用 `SetPocketView`，验证全量刷新立即到达且增量事件持续。
5. 迟到注入：延迟数秒后 `SetPocketView`，验证全量补拉、无空白期。
6. 替换注入：连续两次 `SetPocketView`，验证新视图收到全量刷新、旧视图停止接收。
7. `SetPocketView` 传空：验证停止驱动且无报错；AutoCreate 模式下误调 `SetPocketView` 验证 Warning 且无副作用。
8. 门控回归：专用服务器 + 双客户端，仅本地控制的 Pawn 出现 UI。
9. EndPlay 所有权：External 模式销毁 Pawn，用户自建 HUD 不被移除；AutoCreate 模式销毁，控件从视口移除。

- [ ] **Step 2: 用户验证通过后提交（如 WBP 资产发生重保存）**

```bash
git add Content/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget.uasset
git commit -m "chore(SingularisInventory): 迁移口袋控件资产至视图接口契约"
```

---

## Self-Review 记录

- **Spec 覆盖**：接口（Task 1）、枚举（Task 2）、默认视图（Task 3）、组件属性/API/流程（Task 4）、WBP 迁移与 9 项验证（Task 5）——spec 各节均有对应任务；Editor 模块零变更已写入 Global Constraints。
- **占位符扫描**：无 TBD / TODO；所有代码步骤含完整文件内容；Task 3 cpp 明确"无变更"并给出原因。
- **类型一致性**：`ISingularisPocketViewInterface` 4 函数签名在 Task 1（声明）、Task 3（override）、Task 4（Execute_ 调用）三处逐字一致；`ESingularisPocketViewMode` 取值在 Task 2（定义）与 Task 4（引用）一致；`TScriptInterface` 赋值语义（`operator=(UObject*)`、`operator=(nullptr)`）已对照 UE 5.8 引擎源码 `ScriptInterface.h` 确认。
