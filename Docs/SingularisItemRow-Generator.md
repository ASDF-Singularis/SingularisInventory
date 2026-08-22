# SingularisItemRow 生成器（待实现）

> 状态：**暂缓**。完成其他部分后再解决。本文记录分析与待决策项，供后续动手时翻阅。

## 它是什么

把 DataTable 中的静态数据 → 世界中的形态 Actor + 物品实例的"工厂/桥接器"。**不在既定类图里**，是推断的缺环。

## 要解决的编排流程

设计文档原文：
> 物品进入世界中时，通过数据表中的映射生成具体的形态 Actor，形态 Actor 中必须附加 SingularisItemComponent 组件，最终将物品实例移动给 SingularisItemComponent。当物品从世界中被收容到容器中……销毁形态 Actor，并返还对应的物品实例。

现有拼图：
- `FSingularisMagicalElementRow`（DataTable 行）：持 `ItemClass` + `FormActorClass` + 静态数据
- `USingularisItem`（物品实例）：空壳基类
- `USingularisItemComponent`：`BindItem` / `TakeItem` 已就绪

**缺的一环**：没有一个东西去编排——查表、`NewObject<USingularisItem>`、`SpawnActor<FormActorClass>`、找到其上的 `USingularisItemComponent`、`BindItem`，以及反向 `TakeItem` + `Destroy`。

## 职责清单

1. 入：`FDataTableRowHandle` 或 `RowId`（或已存在的 `USingularisItem*`）
2. 查 `FSingularisMagicalElementRow`
3. 按 `ItemClass` `NewObject` 出物品实例
4. 按 `FormActorClass` `SpawnActor`，确保其上挂了 `USingularisItemComponent`（蓝图配置保证 / 运行时 Attach）
5. `ItemComponent->BindItem(Item)`
6. 出：`ItemComponent->TakeItem()` → `Destroy(FormActor)`

## 载体候选

| 方案 | 说明 | 取舍 |
|---|---|---|
| 静态函数库 `USingularisItemSpawner`（`BlueprintCallable` 静态函数） | 无状态，最轻 | 物品实例所有权/生命周期需调用方管 |
| 子系统 `USingularisItemWorldSubsystem` | 全局唯一，可缓存、可统计、可裁剪 | 重量级，引入全局状态 |
| 组件 `USingularisItemSpawnerComponent` 挂在生成者（玩家/掉落点）上 | 生命周期随所有者，权限清晰 | 每个生成者一份 |

## 待决策项（动手前需拍板）

1. **载体**：静态库 / 子系统 / 组件？
2. **`FormActorClass` 与 `USingularisItemComponent` 的绑定方式**：靠蓝图 BP 默认值预先挂好组件？还是生成器运行时 `FindComponentByClass` 校验甚至动态 `NewObject` 补挂？
3. **生成物品实例的 `Outer`** 是谁？（影响 GC 与复制子对象注册——`BindItem` 里 `AddReplicatedSubObject` 需要物品是有效 UPROPERTY 子对象）
4. **谁拥有物品实例**：生成器临时持有 → `BindItem` 转交 `ItemComponent`？还是物品实例从进入世界起就归 `ItemComponent` 强持有（即当前 `ItemComponent` 的语义）？

## 备选结构

若心里早有别的结构——例如没有独立"生成器"，而是 `SingularisInventoryComponent` 自己负责把物品吐进世界、`SingularisItemComponent` 反向收容——则"生成器"不存在，流程由容器组件编排。需先确认倾向方向再动手。
