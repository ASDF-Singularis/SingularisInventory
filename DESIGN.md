# SingularisInventory 架构设计文档 (Architecture Document)

> [!IMPORTANT]
> SingularisInventory 是 Singularis 系列的两大标准参考实现之一（与 SingularisInteraction 并列）：它以**定义 (Definition) — 实例 (Instance) — 片段 (Fragment) — 形态 (Form)** 四层数据模型承载物品数据，以**逻辑 (Logic) — 表现 (Presentation/UI) — 控制 (Control/Input)** 系列标准模式组织运行时职责，并把静态注册、世界生命周期与容器状态解耦为独立扩展点。

## 概述 (Overview)

`SingularisInventory`（引力奇点库存插件）是 Singularis 系列中面向**物品与容器 (Item & Container)** 领域的运行时框架。插件提供物品的静态定义资产、运行时实例、片段组合、口袋容器、世界形态 Actor、全局注册子系统与视图契约；具体玩法（拾取规则、装备效果、属性结算、快捷栏策略）不在插件内实现。

| 项 | 值 |
| --- | --- |
| 插件名 / FriendlyName | `SingularisInventory` |
| 版本 / 状态 | `0.1.0`，`IsBetaVersion = true`，`IsExperimentalVersion = false` |
| 资产分类 / 作者 | `Singularis` / TrifingZW |
| 许可 | MIT |
| 运行时模块 | `SingularisInventory`（Type = `Runtime`，LoadingPhase = `Default`） |
| 编辑器模块 | `SingularisInventoryEditor`（Type = `Editor`，LoadingPhase = `Default`） |
| 插件级依赖 | `.uplugin` 未声明 `Plugins` 数组（未声明 EnhancedInput 插件依赖，但运行时模块依赖其模块） |
| Singularis 插件间依赖 | 无 |
| 可含内容 | 是（`CanContainContent = true`） |
| 插件级配置 | `Config/DefaultSingularisInventory.ini`（ClassRedirect，源于模块更名） |
| 着色器 | `Shaders/Barrel.usf`，模块启动时映射至 `/Plugin/SingularisInventory` |

运行时模块的模块级依赖（`SingularisInventory.Build.cs`）：

| 分组 | 模块 |
| --- | --- |
| 核心 | `Core`、`CoreUObject`、`Engine`、`NetCore`、`Projects` |
| 渲染 | `RenderCore`、`Renderer`、`RHI` |
| 表现 | `UMG`、`Slate`、`SlateCore` |
| 输入 | `InputCore`、`EnhancedInput` |
| 语义 / 资产 | `GameplayTags`、`AssetRegistry` |
| 配置 | `EngineSettings`、`DeveloperSettings` |

编辑器模块的模块级依赖（`SingularisInventoryEditor.Build.cs`）：`Core`、`CoreUObject`、`Engine`、`SingularisInventory`、`UMG`、`UMGEditor`、`UnrealEd`、`AssetTools`、`ContentBrowser`。

源码目录结构：

```
Plugins/SingularisInventory/
├── SingularisInventory.uplugin
├── Config/
│   └── DefaultSingularisInventory.ini   # ClassRedirect（SingularisInventoryGameplay → SingularisInventory）
├── Content/
│   ├── BP_SingularisItem.uasset
│   ├── DataTables/DT_SingularisInventory_ItemForm.uasset
│   ├── Inputs/{IMC_Singularis_Inventory, Actions/IA_*}
│   ├── Materials/{MI_Barrel, ML_Barrel}
│   ├── Textures/T_Empty.uasset
│   └── UserInterfaces/{WBP_SingularisInventory_SingularisPocketWidget, WBP_SingularisInventory_SingularisPocketSlotWidget}
├── Docs/                                # 设计规格、调研报告与计划（部分与当前代码存在版本落差）
├── Shaders/Barrel.usf
└── Source/
    ├── SingularisInventory/             # Runtime
    │   ├── Public/     # SingularisInventory.h + Actors / Components / Configs / DataAssets / DataTables / Interfaces / Materials / Objects / Subsystems / Types / Widgets
    │   └── Private/    # SingularisInventory.cpp + 与 Public 同名的镜像目录
    └── SingularisInventoryEditor/       # Editor
        ├── Public/     # SingularisInventoryEditor.h + Factories
        └── Private/    # SingularisInventoryEditor.cpp + Factories
```

## 一、 设计目标与边界 (Design Goals & Boundaries)

### 1. 设计目标

| 目标 | 实现手段 |
| --- | --- |
| **三元分离 (Logic — Presentation — Control)** | 逻辑端为口袋 / 物品 / 子系统；表现端为口袋视图接口与控件组件；控制端为挂在 `APlayerController` 上的库存组件（见 2.1） |
| **单一数据源 (SSOT)**：静态配置只存在于定义资产 | `USingularisItemDefinition`（`UPrimaryDataAsset`）聚合标签、形态类、展示数据与片段模板；实例背引用定义，不回写配置 |
| **组合优于继承 (Composition)**：物品能力由片段拼装 | 定义持有平铺 `Instanced` 片段数组，实例物化时逐模板克隆为独立运行时副本 |
| **标签路由 (Tag Routing)**：片段自报响应范围，消费方按标签层级匹配取用 | 片段实现 `IGameplayTagAssetInterface`，`USingularisItem` 提供按类 / 按标签的查询 API |
| **容器与形态解耦 (Container / Form Decoupling)** | 物品实例独立于世界形态 Actor 存在；`USingularisItemComponent` 在形态上强持实例，容器收容时取回实例再销毁形态 |
| **服务器权威 (Server Authority)**：入世界 / 收容 / 丢弃在服务器结算 | `SpawnItemInWorld` / `CollectItem` / `PickupItem` / `DropItem` 标注 `BlueprintAuthorityOnly`；客户端丢弃经 `Server` RPC 上行 |
| **网络复制内建 (Built-in Replication)** | 插槽数组 `ReplicatedUsing` + `COND_OwnerOnly`；物品与片段作为复制子对象注册；客户端经 `OnRep` 快照 diff 触发与权威端等价的事件 |
| **蓝图完整暴露 (Blueprint Parity)** | `BlueprintNativeEvent` 视图 SPI、`BlueprintCallable` / `BlueprintPure` API、中文 `DisplayName` 元数据、五类资产工厂 |

### 2. 边界（非目标）

- 不实现装备、快捷栏、属性结算、商店、合成等具体玩法；片段当前只承载状态与响应标签，消费由项目侧完成。
- 不定义拾取准入规则（距离、权限、队伍）；`USingularisInventoryComponent` 是可选的开箱即用调度器，项目侧可绕过它自行编排。
- 不依赖任何其他 Singularis 插件。
- 不规定 UI 布局与美术；插件提供默认口袋控件与可选图标 / 材质工具，表现由项目侧的视图实现决定。
- 不承担场景查询、交互锁定（由 SingularisInteraction 等插件负责）；两者通过项目侧胶水组合。

## 二、 架构总览 (Architecture Overview)

### 1. 系列标准模式：逻辑 — 表现 — 控制 (Logic — Presentation — Control)

`Singularis` 系列以**逻辑 (Logic) — 表现 (Presentation/UI) — 控制 (Control/Input)** 三元结构作为标准模式。本插件是该模式在库存域的实例，与 SingularisInteraction 并列为系列的标准参考实现。

| 维度 | 职责 | 本插件载体 | 对外契约 |
| --- | --- | --- | --- |
| **逻辑 (Logic)** | 承载物品与容器状态，执行入世界 / 收容 / 插槽变更，不感知输入与 UI | `USingularisPocketComponent`、`USingularisItemComponent`、`USingularisItem`、`USingularisItemFragment`、`USingularisItemDefinition`、`ASingularisItemFormActor`、`USingularisInventorySubsystem` | 口袋六类事件与全量访问器、物品 / 片段查询 API、子系统注册与生命周期原语、`BlueprintAuthorityOnly` 突变 API |
| **表现 (Presentation/UI)** | 消费容器状态并渲染；只读 | `ISingularisPocketViewInterface`、`USingularisPocketWidgetComponent`、`USingularisPocketWidget`、两个默认 WBP；渲染工具 `USingularisMaterialExpressionCurvedScreenUV` 与 `Barrel.usf` | 口袋视图接口：全量刷新（`OnPocketRefresh`）+ 三类增量事件 |
| **控制 (Control/Input)** | 采集输入、把玩家意图转译为容器操作、跨网络边界提交请求 | `USingularisInventoryComponent`、`IMC_Singularis_Inventory` 与 `IA_*` 输入资产 | 增强输入绑定、拾取 / 丢弃 API、`Server_DropItem` RPC、选中为本地行为 |

依赖方向与部署约定（本插件的实现方式）：

- **控制 → 逻辑：** 选中插槽为本地行为（`SelectSlot` 直调）；丢弃经 `Server_DropItem` RPC 上行后由逻辑端结算；拾取 / 收容为逻辑端权威 API。
- **逻辑 → 表现：** “绑定后全量拉取 + 订阅增量事件”的单向推送；框架不向表现端提供回写通路（视图若需改变状态，须自行调用逻辑端公开 API）。
- **表现只观察逻辑端：** 口袋控件组件解析并订阅 `USingularisPocketComponent`；控制组件在本插件中没有对应的视图接口（与 SingularisInteraction 的目标侧 / 玩家侧成对视图不同）。
- **逻辑不引用控制与表现：** 口袋 / 物品 / 片段 / 子系统不含增强输入、控件与摄像机相关代码。

### 2. 分层结构

| 维度 | 层 | 类型 | 说明 |
| --- | --- | --- | --- |
| 共用 | 数据资产层 | `USingularisItemDefinition`、`FSingularisItemFormRow`、`USingularisInventorySettings`、原生 `FGameplayTag` | 静态配置与项目设置；定义资产经 AssetManager 被扫描为主资产 |
| 逻辑 | 对象层 | `USingularisItem`、`USingularisItemFragment` | 运行时实例与片段副本，可被蓝图继承 |
| 逻辑 | 组件层 | `USingularisPocketComponent`、`USingularisItemComponent` | 容器状态与形态绑定 |
| 逻辑 | Actor 层 | `ASingularisItemFormActor` | 物品在世界中的形态基类，默认挂载 `USingularisItemComponent` |
| 逻辑 | 子系统层 | `USingularisInventorySubsystem` | 全局注册表与世界生命周期原语 |
| 控制 | 组件层 | `USingularisInventoryComponent` | 玩家控制器上的调度器与输入绑定 |
| 表现 | 视图层 | 口袋视图接口、口袋控件组件、默认口袋控件与插槽控件 | 状态与事件的消费端，仅依赖接口 |
| 表现 | 渲染工具层 | `USingularisMaterialExpressionCurvedScreenUV`、`Barrel.usf` | 屏幕空间桶形畸变，供 UI / 后期材质使用，不参与状态通路 |

### 3. 核心类职责总表

| 类 / 结构 | 维度 | 基类 | 挂载点 | 职责 |
| --- | --- | --- | --- | --- |
| `USingularisInventorySubsystem` | 逻辑 | `UGameInstanceSubsystem` | GameInstance | 经 AssetManager 构建标签 ↔ 定义 ↔ 形态映射；提供查询、动态注册与入世界 / 收容原语 |
| `USingularisItemDefinition` | 共用 | `UPrimaryDataAsset` | 资产 | 物品静态配置单一数据源：标签、形态类、展示数据、片段模板数组 |
| `USingularisItem` | 逻辑 | `UObject` | 由物化方指定 Outer | 运行时物品实例：背引用定义，持有独立片段副本，参与复制 |
| `USingularisItemFragment` | 逻辑 | `UObject` + `IGameplayTagAssetInterface` | 定义资产内联 / 实例内 | 片段状态载体：自报响应标签，供按类 / 按标签查询 |
| `USingularisPocketComponent` | 逻辑 | `UActorComponent` | 角色的 Pawn | 定容插槽数组、选中状态、六类事件、复制与子对象注册 |
| `USingularisItemComponent` | 逻辑 | `UActorComponent` | 形态 Actor | 强持有物品实例；权威端自动物化；移入 / 取出 / 清除 |
| `ASingularisItemFormActor` | 逻辑 | `AActor` + `ISingularisItemFormActorInterface` | 世界 | 物品形态基类，默认创建 `USingularisItemComponent` 并对外暴露 |
| `USingularisInventoryComponent` | 控制 | `UActorComponent` | `APlayerController` | 输入绑定、拾取 / 丢弃调度、丢弃 RPC 过桥 |
| `USingularisPocketWidgetComponent` | 表现 | `UActorComponent` | Pawn 或 PlayerController | 实例化口袋视图、订阅口袋事件、全量拉取与增量转发 |
| `USingularisPocketWidget` | 表现 | `UUserWidget` + 视图接口 | 视口 | 默认口袋视图，C++ 为空实现，供蓝图子类覆写 |
| `USingularisMaterialExpressionCurvedScreenUV` | 表现 | `UMaterialExpression` | 材质图 | 桶形畸变材质节点；生成与 `Barrel.usf` 等价的 HLSL |
| `USingularisInventorySettings` | 共用 | `UDeveloperSettings` | 项目设置 | 物品实例类与物品形态注册表配置 |

### 4. 运行结构图

```
[定义资产阶段]
USingularisItemDefinition (UPrimaryDataAsset, ItemTag + FormActorClass + Fragments[])
  └─ 项目 AssetManager 扫描（PrimaryAssetType = "SingularisItem"）
       └─ USingularisInventorySubsystem::RebuildRegistry()
            ├─ Tag → Definition / Tag → FormActorClass
            └─ Definition ↔ FormActorClass（经标签桥接推导）

[权威端世界流转]
SpawnItemInWorld(Item, Transform)
  ├─ 查 Definition → FormActorClass → SpawnActor
  ├─ FormActor.ItemComponent->BindItem(Item)（注册复制子对象）
  └─ 根组件开启物理
CollectItem(FormActor)
  └─ ItemComponent->TakeItem() → Destroy(FormActor) → 返回实例

[容器与视图]
USingularisPocketComponent (Pawn)
  ├─ 突变 API（权威端）：AddItem / RemoveItem* / SelectSlot / SwapSlots / Clear
  ├─ 事件（权威端直发；客户端 OnRep diff 补发）
  │    OnItemAdded / OnItemRemoved / OnSelectionChanged
  │    OnSelectedItemChanged / OnItemsSwapped / OnPocketOccupancyChanged
  └─ USingularisPocketWidgetComponent（本地客户端）
       ├─ PocketView.OnPocketRefresh(Capacity, Items, SelectedIndex)   ← 绑定后全量拉取
       └─ PocketView.OnItemAdded / OnItemRemoved / OnSelectionChanged  ← 增量事件

[输入与网络]
USingularisInventoryComponent (PlayerController)
  ├─ IA_FirstPocket..FourthPocket → SelectSlot(i)                [本地]
  ├─ IA_Drop → DropHeldItem() → Server_DropItem RPC              [客户端 → 服务器]
  └─ Server_DropItem → DropItem(Item)：口袋移除 → SpawnItemInWorld
```

### 5. 标签空间 (Tag Namespace)

原生标签在模块内以 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG` 声明（`Types/SingularisInventoryGameplayTags.h` / `.cpp`）：

| 标识符 | 标签 | 用途 |
| --- | --- | --- |
| `SingularisInventory` | `Singularis.Inventory` | 库存域根标签 |
| `SingularisInventory_Item` | `Singularis.Inventory.Item` | 物品标签域根，定义资产的 `ItemTag` 限定于此 |
| `SingularisInventory_Item_Default` | `Singularis.Inventory.Item.Default` | 默认物品标签（代码中无消费方，供项目侧约定使用） |
| `SingularisInventory_Fragment` | `Singularis.Inventory.Fragment` | 片段响应标签域根，片段的 `FragmentTags` 限定于此 |
| `SingularisInventory_Fragment_Default` | `Singularis.Inventory.Fragment.Default` | 默认片段响应标签（代码中无消费方） |

`ItemTag` 与 `FragmentTags` 均带 `Categories` + `ForceSelection = "true"` 元数据，编辑器侧限定在对应子树内取值。

## 三、 核心数据模型 (Data Model)

### 1. 物品定义 (Item Definition)

`USingularisItemDefinition`：`UCLASS(BlueprintType)`，继承 `UPrimaryDataAsset`。每种物品一个定义资产。

| 字段 | 类型 | 语义 |
| --- | --- | --- |
| `ItemType`（静态） | `FPrimaryAssetType` = `"SingularisItem"` | 主资产类型标识，AssetManager 按此类型发现定义 |
| `ItemTag` | `FGameplayTag` | 物品唯一标识，标签 ↔ 形态映射的桥接键 |
| `FormActorClass` | `TSubclassOf<AActor>`（`MustImplement` 形态接口） | 世界形态 Actor 类 |
| `Name` / `Description` | `FText` | 展示名称与描述 |
| `Icon` | `UTexture2D*` | 图标 |
| `Fragments` | `TArray<TObjectPtr<USingularisItemFragment>>`（`Instanced`） | 平铺片段模板数组；注释约定数组顺序即执行顺序 |

`GetPrimaryAssetId()` 返回 `(ItemType, GetFName())`。

### 2. 物品实例 (Item Instance)

`USingularisItem`：`UCLASS(Abstract, BlueprintType, ClassGroup = ("Singularis"), meta = (DisplayName = "引力奇点物品"))`，继承 `UObject`。

| 字段 | 类型 | 复制 | 语义 |
| --- | --- | --- | --- |
| `Definition` | `TObjectPtr<USingularisItemDefinition>` | `Replicated` | 背引用的定义，客户端据此查询静态配置 |
| `Fragments` | `TArray<TObjectPtr<USingularisItemFragment>>` | `Replicated`，`Transient`，`DuplicateTransient` | 从定义模板克隆的独立运行时片段副本 |

`IsSupportedForNetworking()` 返回 `true`；`GetLifetimeReplicatedProps` 复制上述两个字段。

查询 API：`FindFragmentByClass` / `HasFragmentByClass` / `FindFragmentsByClass`（类型匹配，含派生类）、`FindFragmentByTag` / `HasFragmentByTag` / `FindFragmentsByTag`（标签层级匹配）；C++ 便捷模板 `FindFragment<TFragment>()`（`static_assert` 约束片段基类）。

物化 SPI：`static USingularisItem* MaterializeFromDefinition(UObject* Outer, USingularisItemDefinition* ItemDefinition)`，`BlueprintCallable`；行为见 5.2。

### 3. 物品片段 (Item Fragment)

`USingularisItemFragment`：`UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories, ClassGroup = ("Singularis"), meta = (DisplayName = "引力奇点物品片段"))`，继承 `UObject` 并实现 `IGameplayTagAssetInterface`。

| 字段 | 类型 | 语义 |
| --- | --- | --- |
| `FragmentTags` | `FGameplayTagContainer`（`EditDefaultsOnly`，`EditCondition = "bIsCDO"`，`EditConditionHides`） | 片段响应标签，支持层级匹配；`GetOwnedGameplayTags` 默认数据源 |
| `bIsCDO` | `bool`（私有，`Transient`，`DuplicateTransient`，`NonTransactional`） | 编辑器态标记：`PostInitProperties` 中置为“是否为类默认对象” |

关键约定：

- **标签为类级数据：** `CanEditChange` 将 `FragmentTags` 限制为仅在类默认对象（CDO，即蓝图类默认值）上可编辑；定义资产中的内联实例与运行时副本均置灰，继承类默认值。
- **运行时副本独立：** 物化时以定义中的模板为原型逐片段 `NewObject`（拷贝模板配置值作为初始状态），此后实例与定义互不影响。
- **网络：** `IsSupportedForNetworking()` 返回 `true`；基类 `GetLifetimeReplicatedProps` 仅调用 `Super`，供子类追加复制属性。
- **可扩展的响应范围：** C++ 子类可覆写 `GetOwnedGameplayTags` 以动态计算响应标签。
- 当前基类不含触发 / 执行 SPI（见第八章）。

### 4. 口袋插槽与占用状态 (Pocket Slot & Occupancy)

`FSingularisPocketSlot`（`USTRUCT(BlueprintType)`）：单字段 `Item`（`USingularisItem*`）加 `IsEmpty()` 访问器。

`ESingularisPocketOccupancy`（`UENUM(BlueprintType)`）：

| 枚举值 | 显示名 | 判定 |
| --- | --- | --- |
| `Empty` | 空 | 全部插槽为空 |
| `Partial` | 部分占用 | 非空且未满 |
| `Full` | 已满 | 插槽数等于容量且无空槽 |

### 5. 片段上下文 (Fragment Context)

`FSingularisItemFragmentContext`（`Types/SingularisItemFragmentType.h`）：字段为 `Controller`（`AController*`）、`Instigator`（`APawn*`）、`Avatar`（`AActor*`）、`Item`（`USingularisItem*`）、`InputValue`（`FInputActionValue`）。

该结构当前无任何消费方：插件内没有函数接收或构造它；其字段形态（控制器 / 触发者 / 承载者 / 输入值）与触发语义一致（见第八章）。

### 6. 形态注册表行与项目设置 (Form Row & Settings)

`FSingularisItemFormRow`（`FTableRowBase`）：以独立 `ItemTag` 字段为桥接键，映射到 `FormActorClass`（`MustImplement` 形态接口）；注释说明其为“物品定义与形态 Actor 之间的唯一桥梁”。当前运行时代码不读取该结构（见第八章）。

`USingularisInventorySettings`：`UCLASS(Config = SingularisInventory, DefaultConfig)`，继承 `UDeveloperSettings`；编辑器分类 `Singularis`，段落 `Singularis Inventory`，描述“引力奇点库存插件设置”。

| 字段 | 类型 | 默认 | 语义 |
| --- | --- | --- | --- |
| `ItemClass` | `TSubclassOf<USingularisItem>`（`Config`） | 构造期解析 `BP_SingularisItem` | 物化物品实例时使用的具体类；解析失败记录 Error |
| `ItemFormTable` | `TSoftObjectPtr<UDataTable>`（`Config`，`RequiredAssetDataTags` 限定行结构） | 构造期解析 `DT_SingularisInventory_ItemForm` | 物品形态注册表（当前无运行时消费方） |

### 7. 视图实例化模式 (View Instantiation)

本插件的实例化模式以 `USingularisPocketWidgetComponent::bAutoCreateView`（`bool`，默认 `true`）表达：开启时组件按 `PocketWidgetClass` 自动创建并管理视图完整生命周期；关闭时由外部经 `SetPocketView` 注入，组件仅驱动、不拥有。仓库内设计文档曾规划 `ESingularisPocketViewMode` 枚举与视图替换流程，当前代码未采用该枚举（见第八章）。

## 四、 核心模块解析 (Core Module Breakdown)

### 1. `USingularisInventoryComponent`（控制侧调度器）

元数据：`UCLASS(Blueprintable, BlueprintType, ClassGroup = ("Singularis"), meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物库存组件"))`，继承 `UActorComponent`。宿主必须为 `APlayerController`（`BeginPlay` 中 `checkf`）。构造期 `SetIsReplicatedByDefault(true)`，关闭 Tick。

**配置参数**

| 参数 | 默认 | 语义 |
| --- | --- | --- |
| `DropDistance` | `150.0f` | 丢弃位置：角色前方距离 |
| `DropZOffset` | `50.0f` | 丢弃位置：高度偏移 |
| `InputPriority` | `10` | 映射上下文优先级 |
| `InputMappingContext` | 默认资产 `IMC_Singularis_Inventory` | 仅在所控 Character 存在期间挂载 |
| `DropInputAction` | 默认资产 `IA_Drop` | 丢弃输入 |
| `SelectSlotActions` | `IA_FirstPocket`…`IA_FourthPocket` | 数组索引即插槽号，绑定为固定的槽位选择 |

**API**

| 函数 | 权威范围 | 行为 |
| --- | --- | --- |
| `PickupItem(FormActor)` | `BlueprintAuthorityOnly` | 经子系统 `CollectItem` 收容出世界 → 优先放入“选中且为空”的插槽，否则首个空插槽；未入容器时仍返回实例由调用方处置 |
| `DropItem(Item)` | `BlueprintAuthorityOnly` | 从口袋移除（relinquish 持有）→ `SpawnItemInWorld` 生成到角色前方 |
| `DropHeldItem()` | 客户端入口 | 本地读取所控口袋的选中物品 → `Server_DropItem` 上行 |

**网络与输入**

- `Server_DropItem(USingularisItem*)`：`Server` + `Reliable`（未使用 `WithValidation`）。
- 输入绑定仅本地控制器、仅 `ETriggerEvent::Started`；订阅 `OnPossessedPawnChanged`，在 Character 变更时增删映射上下文。
- `EndPlay` 解绑委托并移除映射上下文。

**调度器定位**：类注释明确本组件为“可选的开箱即用调度器”；下层口袋 / 物品 / 子系统保持独立可用，项目侧可自行编排。

### 2. `USingularisPocketComponent`（逻辑侧容器）

元数据：`UCLASS(Blueprintable, BlueprintType, ClassGroup = ("Singularis"), meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点口袋组件"))`，继承 `UActorComponent`。构造期 `SetIsReplicatedByDefault(true)`、`bReplicateUsingRegisteredSubObjectList = true`，关闭 Tick。

**配置参数**

| 参数 | 默认 | 语义 |
| --- | --- | --- |
| `Capacity` | `4`（`ClampMin = 1`） | 最大插槽数，运行时不可变更 |
| `InitialDefinitions` | 空 | BeginPlay 自动物化的初始物品定义；数组索引对应插槽索引，超出容量忽略，空元素留空 |

**状态**

| 成员 | 复制 | 语义 |
| --- | --- | --- |
| `Slots` | `ReplicatedUsing = OnRep_Slots`，`COND_OwnerOnly`，`Transient`，`DuplicateTransient` | 插槽数组，权威端长度恒等于 `Capacity` |
| `SelectedSlotIndex` | 不复制（普通 `int32`） | 本地选中状态：`INDEX_NONE` 无选中；可指向空槽（空手） |
| `PreviousSlotsSnapshot` | 不复制（`Transient`） | 客户端 `OnRep` diff 基线快照 |
| `PreviousOccupancyState` | 不复制 | 占用状态边界检测缓存 |

**事件（6 类）**

| 事件 | 触发条件 |
| --- | --- |
| `OnItemAdded(SlotIndex, Item)` | 指定插槽由空变为非空 |
| `OnItemRemoved(SlotIndex, Item)` | 指定插槽由非空变为空 |
| `OnSelectionChanged(OldIndex, NewIndex)` | 选中索引变化（空选 / 选空槽均合法） |
| `OnSelectedItemChanged(OldItem, NewItem)` | 手持物品变化：选中索引变化的物品差，或选中槽内物品过渡 |
| `OnItemsSwapped(A, B)` | 两个插槽交换物品，供观察者区分“交换”与“先移除再加入” |
| `OnPocketOccupancyChanged(OldState, NewState)` | 占用状态在空 / 部分 / 满之间跨越边界 |

**API**

| 函数 | 语义 |
| --- | --- |
| `IsEmpty` / `IsFull` / `GetOccupancyState` | 状态查询 |
| `GetItem(SlotIndex)` / `GetSelectedIndex` / `HasSelection` / `GetSelectedItem` | 访问器 |
| `AddItem(Item)` | 首个空插槽放入；物品已存在时幂等返回其插槽；满则 `INDEX_NONE` |
| `AddItemAt(Item, SlotIndex)` | 指定插槽放入；要求槽空且物品不在其他槽 |
| `RemoveItem(Item)` / `RemoveItemAt(SlotIndex)` / `RemoveSelectedItem()` | 移除并返还实例 |
| `SelectSlot(SlotIndex)` | 幂等设置选中；`INDEX_NONE` 清空 |
| `SelectNext()` / `SelectPrevious()` | 循环选中，索引回绕 |
| `SwapSlots(A, B)` | 交换两槽物品 |
| `Clear()` | 逐槽移除并广播 |

**复制与事件一致性**

- 权威端：突变 API 直接广播事件，`OnRep_Slots` 提前返回以避免双触发。
- 客户端：`OnRep_Slots` 将 `Slots` 与 `PreviousSlotsSnapshot` 逐槽 diff，经 `BroadcastSlotTransition` 触发等价的加入 / 移除 / 手持变化事件，并更新快照与占用状态。
- 注册 / 注销物品与片段的复制子对象仅发生在权威端；`EndPlay` 防御性注销全部子对象。
- `BeginPlay`：权威端预分配插槽并物化 `InitialDefinitions`；所有端建立快照与占用基线；容量大于 0 时以“下一帧定时器”延迟设置默认选中（首个插槽），确保订阅者完成绑定。

### 3. `USingularisItemComponent`（逻辑侧形态绑定）

元数据：`UCLASS(Blueprintable, BlueprintType, ClassGroup = ("Singularis"), meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点物品组件"))`。构造期 `SetIsReplicatedByDefault(true)`、`bReplicateUsingRegisteredSubObjectList = true`，关闭 Tick；无任何编辑期配置。

| 成员 | 说明 |
| --- | --- |
| `Item` | `Replicated`，`Transient`，`DuplicateTransient`；当前强持有的物品实例 |
| `OnItemBoundEvent` / `OnItemReleasedEvent` | 物品移入 / 取出广播 |

行为要点：

- `BeginPlay`：仅权威端且尚未持有物品时，经 GameInstance → 库存子系统按**自身 Actor 类**反查定义并物化绑定；外部已通过 `BindItem` 填充时以既有实例为准。查询为精确类匹配，不沿继承链。
- `BindItem`：空入参忽略；同一实例幂等；已有其他实例时先注销复制、广播取出再绑定新实例。
- `TakeItem`：解注册、广播取出、清空并返还实例（供收容方接管）。
- `ClearItem`：同取出流程但不返还实例（丢出后不可再拾取等场景）。
- `EndPlay`：防御性注销复制注册并清空引用（不广播取出事件）。
- 注册 / 注销均覆盖物品实例与其片段副本。

### 4. `USingularisItem` 与 `USingularisItemFragment`（运行时数据层）

详见第三章 2 / 3 节。补充要点：

- 实例的 Outer 由物化调用方指定（推荐 `UWorld`，使生命周期脱离形态 Actor / 组件）。
- `MaterializeFromDefinition` 先解析实例类（设置优先），再背引用定义并克隆片段；仅用于权威端 BeginPlay 阶段（注释约定）。
- 片段作为物品实例的复制子对象，由 `USingularisPocketComponent` / `USingularisItemComponent` 在权威端注册。

### 5. `USingularisItemDefinition`（静态定义 SSOT）

详见第三章 1 节。编辑器集成：资产工厂直接在包内 `NewObject` 创建数据资产（非蓝图）；资产行为派生自 `FAssetTypeActions_Base`。

### 6. `USingularisInventorySubsystem`（全局注册与生命周期原语）

元数据：`UCLASS(NotBlueprintable, BlueprintType, ClassGroup = ("Singularis"), meta = (DisplayName = "引力奇点库存子系统"))`，继承 `UGameInstanceSubsystem`。

**注册表（4 张映射）**

| 映射 | 键 → 值 | 构建方式 |
| --- | --- | --- |
| `TagToFormActorMap` | `FGameplayTag` → `TSubclassOf<AActor>` | AssetManager 扫描定义资产时填充（仅当形态类有效） |
| `TagToDefinitionMap` | `FGameplayTag` → 定义指针 | 同上（强引用保持加载） |
| `DefinitionToFormActorMap` | 定义 → 形态类 | 经标签桥接推导 |
| `FormActorToDefinitionMap` | 形态类 → 定义 | 经标签桥接推导 |

**函数**

| 函数 | 语义 |
| --- | --- |
| `Initialize` | 调用 `RebuildRegistry()` |
| `Deinitialize` | 清空四张映射 |
| `FindDefinitionByItemTag` / `FindFormActorClass` | 按标签查询；未命中记录 Warning |
| `FindFormActorClassByDefinition` / `FindDefinitionByFormActorClass` / `FindFormActorClassByItem` | 按定义 / 形态类 / 实例查询 |
| `RegisterItemForm` / `UnregisterItemForm` | 动态注册 / 注销；拆除冲突关联并经标签重建双向映射，保证映射一致 |
| `RebuildRegistry` | 阻塞至资产注册表完成与主资产加载完成，重新扫描构建；蓝图可调用 |
| `SpawnItemInWorld(Item, Transform)` | `BlueprintAuthorityOnly`；查形态类 → `SpawnActor`（`AdjustIfPossibleButAlwaysSpawn`）→ 绑定 `ItemComponent`（缺失则仅入世不可收容）→ 根组件设为 `Movable`、`QueryAndPhysics`、开启物理模拟 |
| `CollectItem(FormActor)` | `BlueprintAuthorityOnly`；查 `ItemComponent` → `TakeItem` → 取回失败则不销毁形态 → 成功则销毁形态并返还实例 |

**注册表构建细节**：`RebuildRegistry` 先 `WaitForCompletion` 等待资产发现（编辑器下异步），清空旧映射，经 `UAssetManager::GetPrimaryAssetIdList(ItemType)` 枚举并以 `LoadPrimaryAssets` + `WaitUntilComplete` 阻塞加载；仅载入 `ItemTag` 有效的定义；最后 `RebuildDefinitionFormMaps` 逐条推导双向映射。

### 7. `ASingularisItemFormActor`（世界形态基类）

`UCLASS(Abstract, Blueprintable)`，继承 `AActor` 并实现 `ISingularisItemFormActorInterface`。

- 构造期 `bReplicates = true`，开启 Tick，创建默认子对象 `USingularisItemComponent`（`ItemComponent`）。
- `ItemComponent` 字段：`EditInstanceOnly` + `BlueprintReadOnly`，允许实例级替换。
- `GetItemComponent_Implementation` 返回该组件；接口 `ISingularisItemFormActorInterface` 仅此一个 `BlueprintNativeEvent` 方法。

### 8. 视图层 (View Layer)

**接口** `ISingularisPocketViewInterface`：`UINTERFACE(Blueprintable, BlueprintType)`，四个 `BlueprintNativeEvent` + `BlueprintCallable` 方法。

| 方法 | 语义 |
| --- | --- |
| `OnPocketRefresh(Capacity, Items, SelectedSlotIndex)` | 全量刷新；绑定完成与视图替换时主动调用，消除错过事件导致的空白期 |
| `OnItemAdded(SlotIndex, Item)` | 增量：物品加入 |
| `OnItemRemoved(SlotIndex, Item)` | 增量：物品移除 |
| `OnSelectionChanged(OldIndex, NewIndex)` | 增量：选中变化 |

**控件组件** `USingularisPocketWidgetComponent`：`UCLASS(Blueprintable, BlueprintType, ClassGroup = ("Singularis"), meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点口袋控件组件"))`；`SetIsReplicatedByDefault(false)`，关闭 Tick。

| 参数 | 默认 | 语义 |
| --- | --- | --- |
| `bAutoCreateView` | `true` | 开启时组件创建并拥有视图 |
| `PocketWidgetClass` | 默认资产 `WBP_SingularisInventory_SingularisPocketWidget` | `MustImplement` 视图接口；`EditCondition = bAutoCreateView` |

| 状态 | 说明 |
| --- | --- |
| `OwnerPlayerController` | 解析得到的本地控制器（弱引用） |
| `PocketView` | `TScriptInterface<ISingularisPocketViewInterface>`，`Transient`；运行时视图缓存 |

生命周期与 API：

- `BeginPlay`：解析本地控制器（Owner 为 Pawn 或 Controller 均可）→ 自动创建视图（可选）→ 解析口袋组件、绑定三个增量事件并全量拉取。
- `EndPlay`：自动创建模式下将控件从视口移除；清空视图引用（事件绑定随组件销毁失效）。
- `SetPocketView`：自动创建模式直接返回；否则幂等写入并立即全量拉取。
- `CreatePocketView`：本地控制器有效 → `CreateWidget` → `ensureMsgf` 运行时复核接口实现 → 缓存并 `AddToViewport`。
- `ResolvePocketComponent`：本地控制器 → 所控 Pawn → `FindComponentByClass`。
- 订阅范围：仅 `OnItemAdded` / `OnItemRemoved` / `OnSelectionChanged` 三类事件；全量刷新携带完整插槽与选中信息。

**默认控件** `USingularisPocketWidget`：`UCLASS(Abstract, Blueprintable)`，实现视图接口，C++ 中四个 SPI 均为空实现；内容侧提供 `WBP_SingularisInventory_SingularisPocketWidget` 与 `WBP_SingularisInventory_SingularisPocketSlotWidget`。

### 9. 渲染工具 (Rendering Utility)

`USingularisMaterialExpressionCurvedScreenUV`：`UCLASS(CollapseCategories, HideCategories = Object)`，继承 `UMaterialExpression`；材质编辑器分类 `Singularis`。

- 输入：`UV`（未连接回退纹理坐标 0）、`CurvatureStrength`（未连接回退 `DefaultCurvatureStrength = 0.1f`）、`AspectRatio`（未连接按视口宽高比 `MEVP_ViewSize` 自动推导）。
- 模型：`r' = r / (1 + k·r²)` 有理分式桶形畸变；中心坐标归一 → x 预乘宽高比 → 径向二次畸变 → 还原宽高比 → 反归一化。
- `Shaders/Barrel.usf` 提供等价函数 `AdvancedCurveScreenUV`；模块 `StartupModule` 将插件 `Shaders/` 目录映射为虚拟路径 `/Plugin/SingularisInventory`。

### 10. 设置与编辑器集成 (Settings & Editor Integration)

编辑器模块 `FSingularisInventoryEditorModule`：`StartupModule` 注册资产分类 `Singularis` 与五个资产行为；`ShutdownModule` 在 `AssetTools` 仍加载时逐一注销并清空缓存数组。

| 资产行为 | 基类 | 目标类 | 工厂产出 | 颜色 | 子菜单 |
| --- | --- | --- | --- | --- | --- |
| `FAssetTypeActions_SingularisItemDefinition` | `FAssetTypeActions_Base` | `USingularisItemDefinition` | `NewObject` 数据资产 | `FColor(255, 168, 46)` | `SingularisInventory` |
| `FAssetTypeActions_SingularisItem` | `FAssetTypeActions_Blueprint` | `USingularisItem` | `UBlueprint` | `FColor(255, 168, 46)` | `SingularisInventory` |
| `FAssetTypeActions_SingularisItemFormActor` | `FAssetTypeActions_Blueprint` | `ASingularisItemFormActor` | `UBlueprint` | `FColor(155, 89, 182)` | `SingularisInventory` |
| `FAssetTypeActions_SingularisItemFragment` | `FAssetTypeActions_Blueprint` | `USingularisItemFragment` | `UBlueprint` | `FColor(0, 168, 255)` | `SingularisInventory` |
| `FAssetTypeActions_SingularisPocketWidget` | `FAssetTypeActions_Blueprint` | `USingularisPocketWidget` | `UWidgetBlueprint` | `FColor(46, 160, 96)` | `SingularisInventory` |

五个工厂（`UFactory` 子类）均设置 `bCreateNew = true`、`bEditAfterNew = true`；蓝图类经 `FKismetEditorUtilities::CreateBlueprint` 创建，数据资产直接在包内 `NewObject`。

## 五、 运行机制与数据流 (Runtime Flow)

### 1. 注册表构建 (Registry Build)

```
GameInstance 初始化
  → USingularisInventorySubsystem::Initialize
      → RebuildRegistry()
          ├─ 等待 AssetRegistry 首轮资产发现完成（编辑器异步）
          ├─ 清空四张映射
          ├─ AssetManager.GetPrimaryAssetIdList("SingularisItem")
          ├─ LoadPrimaryAssets(...).WaitUntilComplete()
          ├─ 逐定义：ItemTag 有效 → TagToDefinitionMap；FormActorClass 有效 → TagToFormActorMap
          └─ RebuildDefinitionFormMaps()：经标签桥接推导定义 ↔ 形态双向映射
```

定义资产的发现范围由项目配置决定；本仓库 `Config/DefaultGame.ini` 的 `AssetManagerSettings` 将 `PrimaryAssetType="SingularisItem"` 的扫描目录指向 `/Game/VehicleTour/ItemDefinitions`。

### 2. 物品物化 (Materialization)

```
USingularisItem::MaterializeFromDefinition(Outer, Definition)
  ├─ 校验 Outer 与 Definition
  ├─ 解析实例类：Settings.ItemClass 优先；未配置或无效时回退 USingularisItem::StaticClass()
  ├─ NewObject<USingularisItem>(Outer, InstanceClass)
  ├─ SetDefinition(Definition)          // 背引用，单一数据源
  └─ InstantiateFragments()             // 逐模板 NewObject 克隆，拷贝配置值作为初始状态
```

### 3. 入世界与收容 (World Lifecycle)

| 方向 | 流程 |
| --- | --- |
| 入世界 `SpawnItemInWorld` | 校验实例 → 经 GameInstance 取 World → 实例定义 → `FindFormActorClassByDefinition` → `SpawnActor` → 找到 `ItemComponent` 则 `BindItem`（注册复制子对象），否则仅入世 → 根组件设为 `Movable` / `QueryAndPhysics` / 开启物理 |
| 收容 `CollectItem` | 校验形态 Actor → 查找 `ItemComponent` → `TakeItem`（解注册 + 广播取出）→ 无物品则不销毁形态 → 成功则 `Destroy(FormActor)` 并返还实例 |

### 4. 容器内流转 (Container Operations)

- 添加：`AddItem` 幂等去重 → 首个空槽 → 写入 → 注册子对象 → 广播 `OnItemAdded` 与占用变化；`AddItemAt` 额外要求目标槽为空。
- 移除：`RemoveItemAt` 解注册 → 清空 → 广播 `OnItemRemoved` 与占用变化 → 返还实例；`RemoveSelectedItem` 复用前者。
- 选中：`SelectSlot` 幂等；索引变化广播 `OnSelectionChanged`，物品变化再广播 `OnSelectedItemChanged`（空槽切空槽不触发）。
- 交换：`SwapSlots` 逐槽广播过渡并额外广播 `OnItemsSwapped`；物品仍在容器内，无需调整子对象注册。
- 清空：`Clear` 逐槽移除并广播。
- 协同：`BroadcastSlotTransition` 在插槽等于选中槽时同步广播 `OnSelectedItemChanged`，保证“选中物品即手持物品”的一致性。

### 5. 拾取与丢弃 (Pickup & Drop)

```
拾取（服务器）：
  PickupItem(FormActor) → CollectItem → 实例
    → 选中槽为空 ? AddItemAt(Item, SelectedIndex) : AddItem(Item)
    → 成功返回实例；未入容器时由调用方处置

丢弃（客户端发起）：
  IA_Drop → DropHeldItem() → 读本地选中实例 → Server_DropItem(Item)
丢弃（服务器结算）：
  Server_DropItem → DropItem(Item)
    → Pocket.RemoveItem(Item)（解除容器持有）
    → SpawnItemInWorld(Item, 角色前方 + Z 偏移)
```

### 6. 视图事件与全量拉取 (View Flow)

| 阶段 | 行为 |
| --- | --- |
| 绑定 | 控件组件解析本地控制器与口袋组件，`AddDynamic` 订阅三个增量事件 |
| 全量拉取 | `OnPocketRefresh(Capacity, Items[0..Capacity-1], SelectedIndex)`；`Items` 由逐槽 `GetItem` 聚合，空槽为 `nullptr` |
| 增量 | `OnItemAdded` / `OnItemRemoved` / `OnSelectionChanged` 逐条经 `Execute_` 转发 |
| 外部注入 | `bAutoCreateView = false` 时 `SetPocketView` 写入后立即全量拉取；替换视图时新视图收到全量刷新 |
| 销毁 | 自动创建模式下控件从视口移除；外部注入的视图生命周期由用户负责 |

### 7. 网络模型 (Network Model)

| 数据 / 动作 | 端 | 机制 |
| --- | --- | --- |
| 口袋插槽 `Slots` | 服务器 → 拥有者客户端 | `ReplicatedUsing = OnRep_Slots`，`COND_OwnerOnly` |
| 选中索引 `SelectedSlotIndex` | 本地 | 普通 `int32`，不复制；各端独立 |
| 快照与占用缓存 | 本地 | `Transient`，不复制 |
| 物品实例 `Definition` / `Fragments` | 服务器 → 客户端 | `DOREPLIFETIME` |
| 形态持有 `ItemComponent.Item` | 服务器 → 客户端 | `DOREPLIFETIME` |
| 物品实例与片段副本 | 服务器 → 客户端 | `bReplicateUsingRegisteredSubObjectList` + `AddReplicatedSubObject` / `RemoveReplicatedSubObject`（仅权威端） |
| 丢弃请求 `Server_DropItem` | 客户端 → 服务器 | `Server` + `Reliable`（未使用 `WithValidation`） |
| 突变 API（拾取 / 丢弃 / 入世界 / 收容） | 服务器 | `BlueprintAuthorityOnly`（仅约束蓝图调用） |
| 形态 Actor | 服务器 → 客户端 | `bReplicates = true` |
| 口袋控件组件 | 本地客户端 | `SetIsReplicatedByDefault(false)` |
| 事件 | 权威端直发 / 客户端 `OnRep` diff 补发 | 六类动态多播委托，未标注 `Replicated` |

### 8. 装配清单 (Integration Checklist)

| 位置 | 必需 | 组件 / 配置 |
| --- | --- | --- |
| 项目设置 | 必需 | AssetManager 扫描 `PrimaryAssetType="SingularisItem"`，指向定义资产目录（本仓库已配置于 `DefaultGame.ini`） |
| 定义资产 | 必需 | `USingularisItemDefinition`：`ItemTag`、`FormActorClass`、`Fragments` |
| 形态蓝图 | 必需 | 继承 `ASingularisItemFormActor`（含 `ItemComponent`）或实现 `ISingularisItemFormActorInterface` 并挂 `USingularisItemComponent` |
| 角色 Pawn | 必需 | `USingularisPocketComponent` |
| 角色 Pawn / 玩家控制器 | 可选 | `USingularisPocketWidgetComponent`（配合 `UWidgetComponent` 或外部注入视图） |
| 玩家控制器 | 可选 | `USingularisInventoryComponent`（开箱即用调度器） |
| 项目设置 | 可选 | `Project Settings → Singularis → Singularis Inventory`：`ItemClass` 需为具体子类，`ItemFormTable` 当前无消费方 |
| Content | 可选 | 默认资产：`BP_SingularisItem`、`IMC_Singularis_Inventory`、`IA_Drop` / `IA_FirstPocket`…`FourthPocket`、两个 WBP |

## 六、 扩展点清单 (Extension Points)

| 扩展点 | 声明位置 | 调用方 | 语义 |
| --- | --- | --- | --- |
| `USingularisItemFragment` 子类 | `Objects/SingularisItemFragment.h` | 项目侧 | 承载物品状态数据与响应标签；可覆写 `GetOwnedGameplayTags` 动态计算响应范围 |
| `USingularisItem` 子类 | `Objects/SingularisItem.h` | 项目侧 + `Settings.ItemClass` | 扩展运行时状态与查询能力 |
| `USingularisItemDefinition` 资产 | `DataAssets/SingularisItemDefinition.h` | 策划 | 新增物品；新增片段模板 |
| `ISingularisPocketViewInterface` | `Interfaces/SingularisPocketViewInterface.h` | `USingularisPocketWidgetComponent` | 自定义口袋表现；实现者可为任意 `UObject` |
| `SetPocketView` | `Components/SingularisPocketWidgetComponent.h` | 项目侧 | 关闭自动创建后注入外部视图 |
| 口袋六类事件 | `Components/SingularisPocketComponent.h` | 蓝图 / C++ 订阅 | 不经过视图接口的旁路监听 |
| `ISingularisItemFormActorInterface` | `Interfaces/SingularisItemFormActorInterface.h` | 子系统 / 项目侧 | 自定义形态 Actor 暴露物品组件 |
| `RegisterItemForm` / `UnregisterItemForm` / `RebuildRegistry` | `Subsystems/SingularisInventorySubsystem.h` | 项目侧 | 运行时动态维护映射与重建注册表 |
| `SpawnItemInWorld` / `CollectItem` | 同上 | 项目侧 | 世界生命周期原语，可绕过调度器自行编排 |
| `USingularisInventorySettings` | `Configs/SingularisInventorySettings.h` | 项目设置 | 指定物品实例类与形态注册表（后者待接线） |
| `USingularisMaterialExpressionCurvedScreenUV` / `Barrel.usf` | `Materials` / `Shaders` | 材质图 | 屏幕空间桶形畸变 |
| 五类资产工厂 | `SingularisInventoryEditor/Factories/*` | 内容浏览器 | 创建定义、物品、形态、片段、口袋控件资产 |

## 七、 系列工程规范 (Series Conventions)

本节提炼自本插件的现有实现。本插件作为 Singularis 系列的标准参考实现（与 SingularisInteraction 并列），以下约定可作为新插件的一致性基线。

### 1. 三元标准模式 (Logic — Presentation — Control)

新插件按系列标准模式划分职责：逻辑端承载领域状态与权威结算，表现端只读消费状态与事件，控制端采集输入并经网络边界提交请求。依赖保持单向（控制 → 逻辑、逻辑/控制 → 表现）：表现端不写回，逻辑端不引用输入与 UI。三元的载体、契约与部署方式见 2.1。

### 2. 模块与目录结构

- 固定两个模块：运行时模块承载全部逻辑，编辑器模块承载资产工厂与资产行为；均 `LoadingPhase = Default`。
- 源文件按 `Public` / `Private` 镜像分目录，目录名使用领域名词复数：`Actors`、`Components`、`Configs`、`DataAssets`、`DataTables`、`Factories`、`Interfaces`、`Materials`、`Objects`、`Subsystems`、`Types`、`Widgets`。
- 类型名与文件名严格同名；`PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs`；引擎头用尖括号、插件头用引号、`.generated.h` 置于末位。
- 插件级 `Config/Default<Plugin>.ini` 承载 `Config` 属性默认值与 `CoreRedirects`（类 / 模块更名兼容）。

### 3. 文件头与许可

- 每个源文件顶部为统一横幅注释块：文件名、`SPDX-License-Identifier: MIT`、`SPDX-FileCopyrightText`、版权行、创建日期与作者、MIT 许可全文。
- 插件根目录放置 `LICENSE.md`、`README.md` 与 `Resources/Icon128.png`。

### 4. 命名规范

| 类别 | 约定 | 示例 |
| --- | --- | --- |
| 类 / 结构体 / 枚举 | 引擎前缀 + `Singularis` + 领域 + 角色 | `USingularisPocketComponent`、`FSingularisPocketSlot`、`ESingularisPocketOccupancy` |
| 委托类型 | `FOn` + 语义 + `Signature`（声明）/ `Event`（成员） | `FOnItemAddedSignature`、`OnItemAddedEvent` |
| 模块 API 宏 | 插件名大写 + `_API` | `SINGULARISINVENTORY_API` |
| 布尔成员 | `b` 前缀 | `bAutoCreateView`、`bIsCDO` |
| 元数据展示名 | 中文 `DisplayName` | `meta = (DisplayName = "引力奇点口袋组件")` |

### 5. 类元数据与分类

- 运行时核心类统一使用 `ClassGroup = ("Singularis")`（本插件中 `USingularisItemDefinition` 与 `ASingularisItemFormActor` 未标注，属现状差异）。
- 可挂载组件使用 `meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点<角色>组件")`，并在构造期显式声明 Tick、复制与子对象列表策略。
- 编辑器可见属性使用中文 `Category`，并允许 `|` 子层级：`Category = "引力奇点物库存组件|输入"`。

### 6. 代码组织与注释

- 头文件与实现文件使用 `#pragma region` 分区，分区顺序：`Parameter` → `Event Dispatcher` → `State` → `Constructors` → `ActorComponent Interface` / `Subsystem Interface` / `UObject Interface` → `API` → `Response` → `RPC` → `Callback` → `Internal Function`。
- 类注释为中文段落，说明角色定位、事件拆分与**不变式 (Invariant)**（如“突变 API 仅权威端调用”）；函数注释含摘要与 `@param` / `@return`。
- 实现函数内部使用编号步骤注释描述执行阶段与守卫原因。

### 7. 蓝图暴露规范

| 场景 | 标注 |
| --- | --- |
| 可被蓝图覆写 / 实现的执行语义 | `BlueprintNativeEvent` + `BlueprintCallable` |
| 无副作用的查询 | `BlueprintPure` |
| 服务器限定逻辑 | `BlueprintAuthorityOnly` |
| 编辑器内联创建的配置对象 | `EditInlineNew` + `DefaultToInstanced` + `Instanced` 属性 + `CollapseCategories` |
| 数据表行结构约束 | `RequiredAssetDataTags = "RowStructure=/Script/<Module>.<Struct>"` |
| 类选择约束 | `meta = (MustImplement = "...")`、`meta = (EditCondition = "...")` |

### 8. 网络编程规范

- 需要复制状态的组件显式 `SetIsReplicatedByDefault(true)`；纯观察者组件显式 `SetIsReplicatedByDefault(false)`。
- 容器状态使用 `ReplicatedUsing` + `OnRep`；权威端直发事件、客户端由 `OnRep` 与本地快照 diff 补发等价事件，避免双触发。
- 子对象复制统一走 `bReplicateUsingRegisteredSubObjectList = true` + `AddReplicatedSubObject` / `RemoveReplicatedSubObject`，登记动作限定在 `HasAuthority()` 端；销毁路径（`EndPlay`）防御性注销。
- 客户端到服务器的请求使用 `Server` + `Reliable`；服务端突变 API 以 `BlueprintAuthorityOnly` 标注。

### 9. 日志与断言分级

本插件固化单一日志分类 `LogSingularisInventory`（模块头声明、模块实现定义），并按下述分级记录：

| 级别 | 适用场景 |
| --- | --- |
| `UE_LOG(Error)` | 默认资产解析失败（构造函数内使用，不配合 `ensure`） |
| `ensureMsgf` | 环境 / 配置缺失导致可安全中断的操作（如控件未实现视图接口、插槽数与容量失配） |
| `UE_LOG(Warning)` | 调用方误用（空入参、非法索引、物品不在容器）与配置缺失 |
| `UE_LOG(Display)` | 正常操作与成功路径追踪 |
| 无日志 | `OnRep` 复制路径（高频噪音豁免） |

消息格式统一为 `[Owner] 函数名：说明`；构造函数内的日志豁免所有者前缀，改用资产路径等静态上下文。

### 10. 防御式编程与幂等

- 所有外部输入函数以 `IsValid` 守卫开路；状态写入函数先做幂等比较，再执行广播与副作用。
- 编辑器选择器约束（`MustImplement`、`EditCondition`）在运行期以 `ensureMsgf` 复核，防止蓝图与 C++ 绕过。
- 跨对象引用使用 `TWeakObjectPtr`（运行时缓存）或 `TObjectPtr`（`UPROPERTY` 成员）。
- 文档与代码同步：设计文档（`Docs/`）记录决策与迁移方案，代码为最终事实来源。

### 11. 内容资源规范

| 前缀 | 类别 | 示例 |
| --- | --- | --- |
| `BP_<Plugin>` | 默认蓝图类 | `BP_SingularisItem` |
| `DT_<Plugin>_<用途>` | 数据表 | `DT_SingularisInventory_ItemForm` |
| `WBP_<Plugin>_<角色>` | 控件蓝图 | `WBP_SingularisInventory_SingularisPocketWidget` |
| `IMC_<用途>` / `IA_<动作>` | 输入资产 | `IMC_Singularis_Inventory` / `IA_Drop` |
| `MI_` / `ML_` / `T_` | 材质实例 / 材质层 / 贴图 | `MI_Barrel` / `ML_Barrel` / `T_Empty` |

## 八、 已知约束与当前实现边界 (Known Constraints)

### 1. 未接线的声明

| 项 | 现状 |
| --- | --- |
| `FSingularisItemFragmentContext` | 已声明（Controller / Instigator / Avatar / Item / InputValue），插件内无任何函数接收或构造；片段触发 / 执行机制未实现 |
| 片段触发 SPI | `USingularisItemFragment` 当前只有标签接口、复制与查询 API，无 `Trigger` / `Execute` 类执行入口；消费方式为项目侧按类 / 按标签查询 |
| `USingularisInventorySettings::ItemFormTable` 与 `FSingularisItemFormRow` | 已声明并可由项目配置（本仓库指向 `DT_VehicleTour_ItemForm`），但运行时代码不读取；映射完全由 AssetManager 扫描定义资产构建 |
| `Content/Inputs/Actions/IA_Fragment` | 资产存在，C++ 中无引用，其消费方未在插件内实现 |
| `SingularisInventory_Item_Default` / `SingularisInventory_Fragment_Default` | 原生标签已定义，插件内无消费方，供项目侧约定使用 |

### 2. 物化与实例化

- `USingularisItem` 为 `Abstract`；`MaterializeFromDefinition` 在 `Settings.ItemClass` 未配置或无效时回退到该抽象基类，而抽象类不可被 `NewObject` 实例化。实际使用必须保持 `ItemClass` 指向具体子类（默认 `BP_SingularisItem`）。
- `USingularisInventorySettings` 构造函数以 `ConstructorHelpers` 解析默认资产；解析失败仅记录 Error 并保留空值，不回退到其他来源。
- `USingularisItemComponent::BeginPlay` 对形态类的定义反查为**精确类匹配**（`TMap` 键为具体 `TSubclassOf`），不沿继承链匹配父类定义。

### 3. 空值守卫与权威范围

- `USingularisInventoryComponent::PickupItem` 与 `DropItem` 经 `GetWorld()->GetGameInstance()->GetSubsystem<...>()` 获取子系统时未做空值守卫（对比：`ItemComponent::BeginPlay` 逐步校验 GameInstance 与子系统，子系统内部亦有守卫）。
- `BlueprintAuthorityOnly` 仅约束蓝图调用；C++ 调用方（含项目侧代码）可绕过该标记直接调用权威 API，函数内部不再复检权威。
- `Server_DropItem` 未实现 `WithValidation` 校验函数，服务端仅按传入实例执行。

### 4. 视图与事件范围

- 口袋控件组件仅订阅 `OnItemAdded` / `OnItemRemoved` / `OnSelectionChanged` 三类事件；`OnSelectedItemChanged`、`OnItemsSwapped`、`OnPocketOccupancyChanged` 不推送至视图（可由项目侧自行订阅）。
- `SetPocketView` 在自动创建模式下静默返回（无日志提示）；外部注入视图的销毁、视口移除与生命周期由调用方负责。
- 视图对口袋组件的解析与事件绑定只在 `BeginPlay` 发生一次；组件挂在 `APlayerController` 且发生 Possess 切换 / 重生时不会自动重新解析（挂在 Pawn 上则随 Pawn 重建）。
- `Slots` 以 `COND_OwnerOnly` 复制：非拥有者客户端不接收容器内容；占用状态等派生数据在复制到达前为初始值。

### 5. 插件元数据与文档落差

- `.uplugin` 未声明 `Plugins` 依赖数组（运行时模块依赖 `EnhancedInput` 模块）；使用本插件的项目需自行确保 EnhancedInput 插件启用。
- 插件级 `Config/DefaultSingularisInventory.ini` 仅含一条类重定向（`SingularisInventoryGameplay.SingularisInventoryComponent` → `SingularisInventory.SingularisInventoryComponent`），表明运行时模块曾更名。
- `Docs/` 内设计文档与代码存在版本落差：`2026-09-09-pocket-view-contract-decoupling-design.md` 描述的 `ESingularisPocketViewMode` 枚举、`TryStartObservation` / `RefreshPocket` 函数与视图替换门控未在代码中落地（代码以 `bAutoCreateView` + `ObservePocketComponent` 实现）；`Lyra-Fragment-Mechanism-Report.md` 与 `SingularisItemRow-Generator.md` 描述的 `TMap<FGameplayTag, Pipeline>` 片段结构、`USingularisItemFragmentComponent::Execute` 与 `Trigger` SPI 亦未在代码中落地。阅读文档时须以源码为准。
- `USingularisItemDefinition` 与 `ASingularisItemFormActor` 未使用 `ClassGroup = ("Singularis")` 及中文 `DisplayName` 元数据；模块头 `SingularisInventory.h` 无类文档注释。

## 九、 关键字字典 (Keywords Glossary)

- **Logic — Presentation — Control (逻辑 — 表现 — 控制)：** Singularis 系列标准模式；逻辑端承载领域状态与权威结算，表现端（UI）只读消费状态与事件，控制端（输入）采集输入并跨网络边界提交请求，依赖单向且表现端不回写。本插件中三者分别由口袋 / 物品 / 子系统、口袋视图接口与控件组件、库存组件承担（见 2.1）。
- **Item Definition (物品定义)：** `USingularisItemDefinition`，`UPrimaryDataAsset`，物品的静态单一数据源；经 AssetManager 以主资产类型 `SingularisItem` 发现。
- **Item Instance (物品实例)：** `USingularisItem`，运行时对象；背引用定义、持有独立片段副本，作为复制子对象同步。
- **Item Fragment (物品片段)：** `USingularisItemFragment`，构成物品能力 / 状态的可组合单元；自报响应标签，当前为状态载体而非执行器。
- **Item Form Actor (物品形态)：** 物品在世界中的 `AActor` 表现，经 `USingularisItemComponent` 强持物品实例；入世生成、收容销毁。
- **Pocket (口袋)：** `USingularisPocketComponent`，定容插槽容器；选中索引承担“手持物品”职责（选中空槽即空手）。
- **Slot Transition (插槽原子过渡)：** `BroadcastSlotTransition` 的语义：单个插槽由旧物品到新物品的过渡，既有又新时先移除后加入，并同步手持变化。
- **OnRep Diff (复制回调差分)：** 客户端在 `OnRep_Slots` 中比较当前值与上一帧快照，补发与权威端等价的增量事件，避免双触发与漂移。
- **Occupancy (占用状态)：** `ESingularisPocketOccupancy`，空 / 部分 / 满三态；仅在跨越边界时广播变化。
- **SSOT (单一数据源)：** 物品静态配置只存在于定义资产；实例与形态均为其派生。
- **Materialization (物化)：** 由定义创建独立运行时实例并克隆片段副本的过程（`MaterializeFromDefinition`）。
- **World Lifecycle Primitives (世界生命周期原语)：** 子系统提供的 `SpawnItemInWorld` / `CollectItem`，只负责物品进出世界，不操纵容器。
- **Registry (注册表)：** 子系统内四张映射（标签→定义、标签→形态、定义→形态、形态→定义），经标签桥接保持双向一致。
- **View Interface (视图接口)：** `ISingularisPocketViewInterface`，表现层唯一依赖契约；实现者可为任意 `UObject`，不限于控件。
- **Full Pull (全量刷新)：** 绑定完成或视图替换后主动调用 `OnPocketRefresh` 推送完整状态，消除错过事件导致的空白期。
- **AutoCreate / External (自动创建 / 外部注入)：** `bAutoCreateView = true` 时组件创建并拥有视图；关闭后由 `SetPocketView` 注入，组件仅驱动、不拥有。
- **Server Authority (服务器权威)：** 入世界 / 收容 / 丢弃在服务器结算；客户端仅经 `Server` RPC 发起丢弃请求。
- **Subobject Replication (子对象复制)：** 以注册列表将物品实例与片段副本纳入复制，仅权威端登记 / 注销。
- **Barrel Distortion (桶形畸变)：** `r' = r / (1 + k·r²)` 屏幕空间 UV 畸变，由材质表达式节点与 `Barrel.usf` 等价实现。
- **Glue (胶水层)：** 项目侧通过定义资产、形态蓝图、容器挂载与视图实现把本插件接入具体玩法的代码。
