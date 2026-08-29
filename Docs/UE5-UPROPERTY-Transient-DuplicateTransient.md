# UE5 UPROPERTY 修饰符速查：Transient / DuplicateTransient

> 用于本项目 `SingularisInventory` 模块的运行时 UObject 引用管理约定。便于遗忘时翻阅。

## 核心结论

两者**正交**，控制两条互不相同的"复制 / 保存"路径。对运行时由服务器生成、靠网络复制同步的 UObject 子对象引用，**成对使用** `Transient, DuplicateTransient`。

| 修饰符 | 控制路径 | 效果 | 不影响 |
|---|---|---|---|
| `Transient` | 序列化（存盘） | 属性在序列化写入时**跳过**，加载时恢复为 C++ 默认值 | GC、网络复制 |
| `DuplicateTransient` | 复制（Duplication） | 对象被复制时，**副本**中该属性重置为 `nullptr` | 序列化 |

## 逐项详解

### `Transient`

- **作用对象**：序列化路径
  - 保存关卡 / SaveGame / PIE 状态快照 / `SaveConfig`
- **效果**：
  - 写盘时跳过该属性
  - 读盘时恢复为 C++ 默认值（指针 → `nullptr`）
- **不**影响：
  - GC：仍是强引用（UPROPERTY 本就持有强引用）
  - 网络复制：由 `Replicated` / `ReplicatedUsing=` 独立控制
- **用途**：不想把"运行时才生成、由复制或运行期逻辑注入"的对象实例写进存档

### `DuplicateTransient`

- **作用对象**：复制路径
  - `DuplicateObject`
  - 编辑器 Duplicate / Copy-Paste Actor
  - **编辑器场景 → PIE 世界**的实例复制
- **效果**：副本中该属性重置为 `nullptr`（不复制原指针）
- **不**影响：序列化（那是 `Transient` 的事）
- **用途**：防止副本"盗用"原对象持有的同一 UObject 子对象指针，避免两个 Actor 共享同一 item 实例引用导致的脏状态 / GC 串味

## 成对使用的必要性

对一个"运行时由服务器生成、靠 replication 同步、靠 `AddReplicatedSubObject` 注册"的 UObject 引用（如 `USingularisItemComponent::Item`、`USingularisPocketComponent::Slots` 中的 Item）：

| 路径 | 仅 `Transient` | 仅 `DuplicateTransient` | `Transient, DuplicateTransient` |
|---|---|---|---|
| 保存关卡 / SaveGame | 不写入 ✅ | 会写入 ❌ | 不写入 ✅ |
| 编辑器 Duplicate / → PIE 副本 | 复制原指针 ❌ | 副本置空 ✅ | 副本置空 ✅ |

- 只加 `Transient`：编辑器 Duplicate 或进 PIE 时，副本带上原实例陈旧指针 → 副本启动后引用一个"不属于它"的 item，触发脏状态甚至崩溃。
- 只加 `DuplicateTransient`：存档里仍会写进该指针，加载回来后指向一个未必存在的对象。
- 组合用 = "这个引用纯属运行期持有，既不落盘，也不随副本迁移，由复制系统在运行期重新注入。"

## 与 `Replicated` 的关系

`Replicated` / `ReplicatedUsing=` 是**网络复制**开关，与前两者完全独立：

- 运行期注入的引用配合 `AddReplicatedSubObject` + `DOREPLIFETIME` 完成跨端同步
- `Transient, DuplicateTransient` 只保证它不会从存档 / 副本里"鬼祟地"冒出来

## 本项目典型写法

```cpp
// ItemComponent：单实例强持有
UPROPERTY(Replicated, Transient, DuplicateTransient)
TObjectPtr<USingularisItem> Item = nullptr;

// PocketComponent：含子对象指针的复制数组
UPROPERTY(ReplicatedUsing = OnRep_Slots, Transient, DuplicateTransient)
TArray<FSingularisPocketSlot> Slots{};

// 仅用于客户端 OnRep diff 的快照（不复制、不存盘、不迁移）
UPROPERTY(Transient)
TArray<FSingularisPocketSlot> PreviousSlotsSnapshot{};
```

## 相关易混修饰符（顺带对照）

| 修饰符 | 作用 |
|---|---|
| `Replicated` | 网络复制该属性（无 OnRep 回调） |
| `ReplicatedUsing=Fn` | 网络复制 + 收到回调时调用指定函数（`OnRep_` 命名不会自动绑定，必须显式指定） |
| `Transient` | 不序列化 |
| `DuplicateTransient` | 副本中置空 |
| `NonTransactional` | 不进入编辑器 undo/redo 事务缓冲 |
| `SkipSerialization` | 现代写法别名，等价于不参与序列化 |
| `Instanced` | 该 UObject 属性按"实例化子对象"处理（编辑器内联编辑、独立复制） |

## 何时只用 `Transient`（不加 `DuplicateTransient`）

属性满足以下任一条件即可：
- 值由对象自身在 `BeginPlay` / `PostInitProperties` 重新初始化，副本重置为空无所谓
- 是纯运行期缓存（如本项目 `PreviousSlotsSnapshot` 这种 diff 基线快照）

## 何时必须成对

满足以下任一条件：
- 引用运行时生成的 UObject 子对象，且其所有权属于该对象本身
- 该引用若被副本或存档"复刻"出来，会指向错误 / 失效的对象
