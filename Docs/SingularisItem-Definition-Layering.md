# SingularisInventory 物品定义分层

> 记录物品定义形态的设计决策。通用背景见 `UE5-PrimaryDataAsset-ItemDefinition.md`。

## 结论

- **UPrimaryDataAsset** 是 **UObject** 的子类。基类选择依据物品定义的形态。
- 物品定义采用三层分离：**Definition / Fragment / Instance**。
- `USingularisItem` 是运行时实例，使用 **UObject**。
- 定义数据位于 `FSingularisItemRow`（**DataTable** 行），建议迁移为 **UPrimaryDataAsset** 资产。

## 现状

| 类 | 文件 | 角色 | 判定 |
|---|---|---|---|
| `USingularisItem` | `Public\Objects\SingularisItem.h` | 运行时实例，含 `MaterializeFromTemplate` | 保留为 Instance 层 |
| `FSingularisItemRow` | `Public\DataTables\SingularisItemRow.h` | 定义数据：`ItemClass`、`FormActorClass` | 迁移为定义资产 |
| `USingularisItemAction` | `Public\Objects\SingularisItemAction.h` | 动作片段（**EditInlineNew**） | 保留为 Fragment 层 |

依据：`SingularisItemRow-Generator.md` 将 `USingularisItem` 定义为"物品实例"；组件以 `Replicated, Transient, DuplicateTransient` 的 `TObjectPtr<USingularisItem>` 持有。

## 分层

继承链：**UObject** → **UDataAsset** → **UPrimaryDataAsset**。

| 层级 | 基类 | 生命周期 |
|---|---|---|
| Definition | **UPrimaryDataAsset** | 持久资产 |
| Fragment | **UObject** + **EditInlineNew** | 随定义资产 |
| Instance | **UObject** | transient |

## 对比

| 能力 | **DataTable** 行 + `TSubclassOf` | **UPrimaryDataAsset** |
|---|---|---|
| 内容浏览器编辑 | 需改表 | 独立资产 |
| 引用修复 | 重命名、移动需处理 | **Redirector** 自动 |
| **Asset Manager** 集成 | 无 | `FPrimaryAssetId` |
| 软引用异步加载 | 整表加载 | `TSoftObjectPtr` + `RequestAsyncLoad` |
| **Asset Bundle** | 无 | `AssetBundleData` |
| 行为扩展 | 新建 **Blueprint** 子类 | **Instanced** 片段 |

## 迁移方案

### 方案 A：保持现状

`USingularisItem` 保持为 **UObject** 实例，不作为定义。

### 方案 B：定义资产化（推荐）

1. 新增 `USingularisItemDefinition : UPrimaryDataAsset`，承载 `FSingularisItemRow` 的数据。
2. 保留 **DataTable** 时，`TSubclassOf<USingularisItem> ItemClass` 改为 `TSoftObjectPtr<USingularisItemDefinition>`。
3. `USingularisItem` 持有 `TObjectPtr<USingularisItemDefinition>`。
4. `MaterializeFromTemplate` 的模板来源改为定义资产。

### 方案 C：移除 DataTable

- 使用 **Asset Manager** 注册 `PrimaryAssetType` 扫描定义资产。
- 使用 `GetPrimaryAssetIdList` 枚举定义。

## Lyra 对应

| Lyra | 本模块 |
|---|---|
| `ULyraInventoryItemDefinition : UDataAsset` | `USingularisItemDefinition : UPrimaryDataAsset` |
| `ULyraInventoryItemFragment : UObject`（**EditInlineNew**） | `USingularisItemAction` |
| `ULyraInventoryItemInstance : UObject` | `USingularisItem` |

## 不变项

- `USingularisItem`：**UObject** 实例，`Transient, DuplicateTransient` 引用约定。
- `USingularisItemAction`：**UObject** + **EditInlineNew** 片段。
- `MaterializeFromTemplate` 物化流程。
