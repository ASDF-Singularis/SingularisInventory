# 引力奇点口袋视图契约解耦设计

## 概述

将 `USingularisPocketWidgetComponent` 对具体 Widget 基类 `USingularisPocketWidget` 的硬依赖解耦为 `UInterface` 视图契约，并提供双路径实例化：自动创建（现状）与外部注入（新增）。通用设计规则见主工程 VehicleTour 仓库 `Docs/widgetcomponent-widget-contract-decoupling.md`（项目无关指导，跨插件共用），本文档只记录本插件的落地设计。

## 决策记录

| 决策点 | 结论 | 说明 |
|--------|------|------|
| 接口事件覆盖 | 最小同构（4 函数） | 与现有 Widget SPI 完全一致：全量刷新 + 加入 + 移除 + 选中变化；后续扩展接受破坏性变更代价 |
| 接口命名 | `ISingularisPocketViewInterface` | 用 View 而非 Widget：实现者未必是控件 |
| 落点 | `Interfaces/SingularisPocketViewInterface.h/.cpp` | 与 `SingularisItemFormActorInterface` 同目录、同声明范式 |
| 默认视图 | `USingularisPocketWidget` 保留，去除 `Abstract` | 契约转移至接口后，它成为默认实现而非抽象契约 |
| 实例化模式 | 显式枚举 `ESingularisPocketViewMode` | AutoCreate（默认）/ External；避免"默认类已由 ConstructorHelpers 配置、外部注入用户忘记清空导致默认控件闪现后被替换"的隐式冲突 |
| 类属性放宽 | `TSubclassOf<UUserWidget>` + `MustImplement` | 自动创建路径也可使用用户自定义 Widget 类 |
| 组件类名 / DisplayName | 不变 | 限制变更半径 |

## 变更文件清单

| 文件 | 操作 |
|------|------|
| `Source/SingularisInventory/Public/Interfaces/SingularisPocketViewInterface.h` | 新建：视图契约接口 |
| `Source/SingularisInventory/Private/Interfaces/SingularisPocketViewInterface.cpp` | 新建：仅含 include（同构 `SingularisItemFormActorInterface.cpp`） |
| `Source/SingularisInventory/Public/Types/SingularisPocketType.h` | 修改：新增 `ESingularisPocketViewMode` |
| `Source/SingularisInventory/Public/Widgets/SingularisPocketWidget.h` | 修改：继承接口、去 `Abstract` |
| `Source/SingularisInventory/Private/Widgets/SingularisPocketWidget.cpp` | 无变更：SPI 声明位于头文件，cpp 函数定义签名不变 |
| `Source/SingularisInventory/Public/Components/SingularisPocketWidgetComponent.h` | 修改：双路径重构 |
| `Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp` | 修改：双路径重构 |
| `Content/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget` | 迁移：重编译重保存 |
| `SingularisInventoryEditor` 模块 | 无变更（工厂与资产类型操作引用的类持续存在） |

## 详细设计

### 1. 视图契约接口

```cpp
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
     * 由 WidgetComponent 在绑定完成与视图替换时主动调用，消除错过事件导致的空白期。
     * 外部注入的视图必须预期本函数在其被设置后立即到达。
     */
    UFUNCTION(
        BlueprintNativeEvent,
        BlueprintCallable,
        Category = "SingularisInventory|引力奇点口袋视图接口|SPI",
        meta = (DisplayName = "口袋刷新")
    )
    void OnPocketRefresh(int32 Capacity, const TArray<USingularisItem*>& Items, int32 SelectedSlotIndex);

    UFUNCTION(... meta = (DisplayName = "物品加入"))
    void OnItemAdded(int32 SlotIndex, USingularisItem* Item);

    UFUNCTION(... meta = (DisplayName = "物品移除"))
    void OnItemRemoved(int32 SlotIndex, USingularisItem* Item);

    UFUNCTION(... meta = (DisplayName = "选中变化"))
    void OnSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex);

#pragma endregion
};
```

要点：

- 签名、`BlueprintNativeEvent` + `BlueprintCallable` 组合与现有 Widget SPI 逐字一致，仅类别与文件归属变化。
- `USingularisItem` 前置声明，与 `SingularisItemFormActorInterface` 同构。
- 4 个函数语义不变，`OnPocketRefresh` 文档补充"视图替换时也会调用"。

### 2. 实例化模式枚举

落点 `Types/SingularisPocketType.h`：

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

### 3. 默认视图 `USingularisPocketWidget`

```cpp
UCLASS(Blueprintable)
class SINGULARISINVENTORY_API USingularisPocketWidget : public UUserWidget, public ISingularisPocketViewInterface
{
    GENERATED_BODY()

public:
#pragma region SPI

    virtual void OnPocketRefresh_Implementation(
        int32 Capacity,
        const TArray<USingularisItem*>& Items,
        int32 SelectedSlotIndex
    ) override;
    virtual void OnItemAdded_Implementation(int32 SlotIndex, USingularisItem* Item) override;
    virtual void OnItemRemoved_Implementation(int32 SlotIndex, USingularisItem* Item) override;
    virtual void OnSelectionChanged_Implementation(int32 OldSlotIndex, int32 NewSlotIndex) override;

#pragma endregion
};
```

要点：

- 去除 `Abstract`；4 个 `_Implementation` 保留空实现，实际逻辑由 WBP 子类覆写（现状不变，仅继承来源从本类变为接口）。
- 构造、生命周期无变化。

### 4. `USingularisPocketWidgetComponent`

#### 4.1 属性变更

| 属性 | 变更前 | 变更后 |
|------|--------|--------|
| `PocketWidget` | `TObjectPtr<USingularisPocketWidget>`，`EditInstanceOnly` | 删除，由 `PocketView` 取代 |
| `PocketView` | 无 | `TScriptInterface<ISingularisPocketViewInterface>`，`Transient` + `BlueprintReadOnly`，Instantiation region |
| `PocketViewMode` | 无 | `ESingularisPocketViewMode`，`EditDefaultsOnly`，默认 `AutoCreate`，Parameter region |
| `PocketComponentReference` | 不变 | 不变 |
| `PocketWidgetClass` | `TSubclassOf<USingularisPocketWidget>`，`EditDefaultsOnly` | `TSubclassOf<UUserWidget>`，`meta=(MustImplement="SingularisPocketViewInterface", EditCondition="PocketViewMode == ESingularisPocketViewMode::AutoCreate")`，DisplayName 改为"口袋视图类" |

`PocketView` 不设 `EditInstanceOnly` 的原因：编辑器面板赋值不会触发绑定逻辑，形成绕过 `SetPocketView` 的第二写入口；写入仅经 `SetPocketView` 单入口（单一数据源）。

`PocketWidgetClass` 类型放宽不破坏序列化：`TSubclassOf<UUserWidget>` 是原类型的超集，已配置的 WBP 引用继续有效。

#### 4.2 新增内部状态

```cpp
#pragma region Internal Variable

/** 口袋事件绑定去重守卫。 */
bool bBound = false;

/** 绑定时解析的口袋组件缓存，供视图替换后的补刷新使用。 */
TWeakObjectPtr<USingularisPocketComponent> ResolvedPocketComponent = nullptr;

#pragma endregion
```

不设"是否拥有视图"标记：所有权由 `PocketViewMode` 唯一决定（`SetPocketView` 仅在 External 模式可用，External 模式永不自动创建），无需冗余状态。

#### 4.3 新增 API

```cpp
/**
 * 设置外部口袋视图（仅 External 模式）。
 * 立即触发绑定与全量拉取；重复调用视为替换视图，新视图收到一次全量刷新，旧视图自然停止接收事件。
 * 传空表示停止驱动。
 */
UFUNCTION(BlueprintCallable, Category = "SingularisInventory|引力奇点口袋控件|API", meta = (DisplayName = "设置口袋视图"))
void SetPocketView(const TScriptInterface<ISingularisPocketViewInterface>& NewPocketView);
```

`SetPocketView` 流程：

1. 卫语句：`PocketViewMode != External` → Warning 日志并返回（契约显式，避免与自动创建路径冲突）。
2. 写入 `PocketView`（空合法）。
3. 若 `bBound` 且新视图有效且 `ResolvedPocketComponent` 有效 → `RefreshPocket`（新视图补全量状态）。
4. 若 `!bBound` → `TryStartObservation()`（处理视图先于 / 晚于组件 BeginPlay 的两种时序）。

#### 4.4 内部函数重排

```cpp
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
```

`CreatePocketView`（原 `CreatePocketWidget` 调整）：

1. 解析本地 PlayerController，无效返回（Display 已记录）。
2. `PocketWidgetClass` 无效返回（Warning）。
3. `CreateWidget<UUserWidget>`，失败 `ensureMsgf`。
4. 零信任校验 `ImplementsInterface(USingularisPocketViewInterface::StaticClass())`：`MustImplement` 仅约束编辑器选择器，C++ / 蓝图图赋值可绕过；失败 Error 日志并跳过。
5. 写入 `PocketView`，`AddToViewport`。

`TryStartObservation`（新增核心）：

1. 卫语句：`bBound` → 返回。
2. 卫语句：`PocketView.GetObject()` 无效 → 静默返回（External 模式视图迟到属预期状态，非错误）。
3. 卫语句：本地 PlayerController 无效 → 返回（Display 已记录）。
4. 解析口袋组件，无效 → Warning 返回（引用为同 Actor 组件，BeginPlay 后必已存在，失败即配置错误）。
5. 绑定三个事件（`AddDynamic`），缓存 `ResolvedPocketComponent`，置 `bBound = true`。
6. `RefreshPocket`。

`RefreshPocket`：聚合逻辑不变，推送改为 `ISingularisPocketViewInterface::Execute_OnPocketRefresh(PocketView.GetObject(), ...)`。

`HandleItemAdded` / `HandleItemRemoved` / `HandleSelectionChanged`：结构不变，守卫改为 `PocketView.GetObject()` 有效，转发改为 `Execute_` 调用。

#### 4.5 生命周期

`BeginPlay`：

1. `PocketViewMode == AutoCreate` → `CreatePocketView()`。
2. `TryStartObservation()`。

`EndPlay`（顺序保持：先移除后 Super）：

1. `PocketViewMode == AutoCreate` 且视图有效 → `RemoveFromParent()`（屏幕空间控件不随组件销毁自动移除）。
2. 两种模式统一清空 `PocketView` 与 `ResolvedPocketComponent`（事件绑定随组件销毁自动失效，无需显式解绑）。External 模式除清空引用外不做任何生命周期操作。

## 所有权契约

| 操作 | AutoCreate | External |
|------|-----------|----------|
| 创建 / 视口挂载 | 组件 | 用户 |
| 视口移除 / 销毁 | 组件 | 用户 |
| 事件驱动 / 全量拉取 | 组件 | 组件 |
| 替换视图 | 不支持（配置期决定） | `SetPocketView`，新视图收到全量刷新 |

## 兼容性与迁移

1. `PocketWidget` 属性删除：蓝图图中对它的读取需改绑 `PocketView`（经 Get Object 取实例）。原属性为只读暴露，改bind 成本低。
2. `PocketWidgetClass` 放宽：序列化数据兼容，已配置组件无需处理。
3. `PocketViewMode` 默认 `AutoCreate`：存量组件行为与重构前逐项一致（创建 → 视口 → 绑定 → 拉取 → 转发），零配置变更。
4. WBP 资产：4 个事件覆写的归属从父类函数变为父类实现的接口函数。编辑器重编译后打开资产逐一确认覆写仍在；若个别事件节点丢失绑定，重新覆写（C++ 侧为空实现，真实逻辑全在 WBP 内，可对照事件名恢复）。
5. Editor 模块：`USingularisPocketWidgetFactory` 与资产类型操作引用的 `USingularisPocketWidget` 持续存在，零变更。

## 验证方案

约束：不在本 IDE 编译。以下在编辑器内人工执行：

1. 模块编译通过后打开 WBP 资产，重编译重保存，确认 4 个事件覆写存在。
2. 路径 A 回归：默认配置（AutoCreate + 默认类）PIE，控件出现，物品加入 / 移除 / 选中切换正确反映。
3. 路径 B 基础：自建 HUD Widget 实现接口（类设置勾选接口），External 模式，BeginPlay 末尾 `SetPocketView`，验证全量刷新立即到达且增量事件持续。
4. 迟到注入：延迟若干秒后 `SetPocketView`，验证全量补拉无空白期。
5. 替换注入：连续两次 `SetPocketView`，验证新视图收到全量刷新、旧视图停止接收。
6. `SetPocketView` 传空：验证停止驱动且无报错。
7. 门控回归：专用服务器 + 双客户端，仅本地控制的 Pawn 出现 UI，模拟客户端无幽灵控件。
8. EndPlay 所有权：External 模式下销毁 Pawn，验证用户自建 HUD 不被移除；AutoCreate 模式下销毁，验证控件从视口移除。
9. AutoCreate 模式误调 `SetPocketView`：验证 Warning 且无副作用。

## 风险

| 风险 | 缓解 |
|------|------|
| WBP 事件覆写在函数迁移后丢失 | 验证方案第 1 步逐一确认；丢失则按事件名重建（共 4 个，C++ 侧空实现，WBP 持有全部真实逻辑） |
| `MustImplement` 约束被 C++ / 蓝图图赋值绕过 | `CreatePocketView` 第 4 步运行时零信任校验 |
| `TScriptInterface` 蓝图互操作生僻（读取需经 Get Object） | 文档注释说明；`SetPocketView` 输入引脚即接口类型，蓝图用户无需手动构造 |
