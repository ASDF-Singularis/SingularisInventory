# SingularisInventory 日志与断言基础设施设计

日期:2026-08-21
状态:已批准(方案 B:插桩 + 分级约定文档)

## 背景与目标

插件当前全源码**零 UE_LOG/check/ensure**,所有失败路径静默返回(nullptr/false),失败不可见、不可诊断。本设计为插件建立日志与断言基础设施,并固化为长期约定:

1. 让所有失败可见、可定位(级别、调用栈、上下文)
2. 让正常操作可追踪(Verbose 级调试流)
3. 为未来新增代码提供明确的分级规则

## 已确认决策

| 决策点 | 结论 |
|---|---|
| 断言强度 | 混合策略:编程不变量用 check;可恢复的外部失败用 ensureMsgf |
| 日志范围 | 失败路径 + 正常操作(Verbose) |
| 分类粒度 | 单一分类 `LogSingularisInventory`(编辑器模块复用) |
| 宏封装 | 直接使用 UE 内建宏,不封装自定义宏层 |
| 组织方式 | 方案 B:插桩 + 本分级约定文档 |

## 日志分类

- 分类名 `LogSingularisInventory`,默认级别 `Log`,编译上限 `All`
- 声明:`DECLARE_LOG_CATEGORY_EXTERN(LogSingularisInventory, Log, All)` 于 `Source/SingularisInventory/Public/SingularisInventory.h`
- 定义:`DEFINE_LOG_CATEGORY(LogSingularisInventory)` 于 `Source/SingularisInventory/Private/SingularisInventory.cpp`
- 编辑器模块复用该分类(编辑器模块依赖运行时模块,且保持单一分类名)

## 严重级别分级策略(核心约定)

| 级别 | 适用场景 | 机制 |
|---|---|---|
| **check/checkf** | 编程不变量——违反后继续执行必然崩溃/未定义行为 | 开发版崩溃暴露。当前代码无适用点,为未来代码预留 |
| **ensureMsgf(Error)** | 环境/资产/系统缺失、内部不变量被破坏——低频、严重、可安全中断该操作 | 记录 Error 日志 + 调用栈(不崩溃),失败即 return;不再另写 UE_LOG |
| **Warning** | 调用方误用(空入参、非法索引)、配置缺失但可预期(数据表缺行、引用未配置)——可能中高频 | UE_LOG Warning,不弹窗不刷调用栈 |
| **Verbose** | 正常操作全流程追踪(物品出入世界、插槽增删换、选中切换、成功路径) | UE_LOG Verbose,运行时默认不显示,`-LogCmds` 开启 |

**判定规则:**

- 预计**高频**的失败(如每帧查表缺行)→ 只用 Warning,禁用 ensure(避免弹窗/刷屏)
- 预计**低频**的失败(默认资产缺失、子系统缺失、生成 Actor 失败)→ ensureMsgf,调用栈直接指向问题源头
- 空入参/非法索引是 BP 调用方错误 → Warning(帮助发现连线错误,不惩罚)
- 正常玩法条件(口袋已满、空槽取出、不可收容物品)→ Verbose
- OnRep 复制路径不记日志(高频噪音)
- 构造函数内不使用 ensure,仅用 UE_LOG Error

## 逐失败点映射(20 处)

**总原则:每个失败出口都留一条日志;根因层(子系统)与操作层(组件)各自记各自的,不纠结重复。**

### USingularisInventoryComponent — 8 处

| 位置 | 条件 | 级别 |
|---|---|---|
| SpawnItemInWorld | 空物品入参 | Warning |
| SpawnItemInWorld | GetWorld() 为 null | ensureMsgf + return(伴随修正,见下) |
| SpawnItemInWorld | GameInstance / 子系统无效 | ensureMsgf + return |
| SpawnItemInWorld | FormActorClass 无效(数据表缺行) | Warning(含物品类名) |
| SpawnItemInWorld | SpawnActor 失败 | ensureMsgf + return |
| SpawnItemInWorld | 成功 | Verbose(物品类名 + Actor 名 + 位置) |
| CollectItem | 空 FormActor 入参 | Warning |
| CollectItem | 无 ItemComponent(设计上的"不可收容")/ 无物品 | Verbose |
| CollectItem | 成功 | Verbose(物品名 + 入容器结果) |

### USingularisItemComponent — 3 处

| 位置 | 条件 | 级别 |
|---|---|---|
| BindItem | 空入参 | Warning |
| BindItem | 幂等命中 / 替换旧实例 / 成功 | Verbose |
| TakeItem | 空状态(正常)/ 成功 | Verbose |

### USingularisPocketComponent — 12 处

| 位置 | 条件 | 级别 |
|---|---|---|
| AddItem | 空入参 | Warning |
| AddItem | 已存在(幂等)/ 口袋满(正常玩法条件) | Verbose |
| AddItem | 权威端 Slots/Capacity 失配(运行时改 Capacity 未重初始化) | ensureMsgf |
| AddItemAt | 空入参 / 非法索引 / 槽已占用 / 物品重复(调用方误用) | Warning |
| RemoveItem | 空入参 / 物品未找到 | Warning |
| RemoveItemAt | 非法索引 → Warning;空槽(正常边界) | Verbose |
| SelectSlot / SelectNext / SelectPrevious / SwapSlots | 非法索引、Capacity≤0、相同索引(误用) → Warning;幂等/成功 → Verbose | Warning / Verbose |
| Clear / BeginPlay 初始化 | 成功 | Verbose |
| OnRep_* 路径 | — | 不记 |

### USingularisPocketWidgetComponent — 6 处

| 位置 | 条件 | 级别 |
|---|---|---|
| 构造函数 | 默认 WBP 加载失败 | Error(UE_LOG,含完整路径) |
| ResolveOwningLocalPlayerController | 非本地控制(其他玩家复制 Pawn,预期) | Verbose |
| CreatePocketWidget | 非本地 PC → Verbose;控件类无效 → Warning;CreateWidget 失败 → ensureMsgf | Verbose / Warning / ensureMsgf |
| ObservePocketComponent | 引用未解析 / 非口袋组件 | Warning(配置问题) |
| 绑定成功 + 全量拉取 | 成功 | Verbose |

### USingularisInventoryItemSubsystem — 2 处

| 位置 | 条件 | 级别 |
|---|---|---|
| GetItemTable | 设置/数据表无效 | Warning(提示去项目设置配置) |
| FindItemRowByClass | 表有效但行未找到 | Warning(含物品类名,"请检查物品数据配置") |

查询层空入参不记(高频 const 查询,组件层已记)。

### USingularisInventorySettings — 1 处

| 位置 | 条件 | 级别 |
|---|---|---|
| 构造函数 | 默认数据表加载失败 | Error(UE_LOG,含路径) |

## 消息格式约定

- 中文消息,陈述式,与现有注释风格一致
- 统一模式:`上下文 | 动作 | 参数`;上下文用 `GetNameSafe(GetOwner())`、`GetNameSafe(Item)`、物品类名、槽位索引
- ensureMsgf 形态:`ensureMsgf(LogSingularisInventory, Cond, TEXT("..."))`,失败即 return
- Warning/Verbose 用 `UE_LOG` + `%s`/`%d` 参数

## 改动文件清单

1. `Source/SingularisInventory/Public/SingularisInventory.h` — 声明分类
2. `Source/SingularisInventory/Private/SingularisInventory.cpp` — 定义分类
3. `Source/SingularisInventory/Private/Components/SingularisInventoryComponent.cpp`
4. `Source/SingularisInventory/Private/Components/SingularisItemComponent.cpp`
5. `Source/SingularisInventory/Private/Components/SingularisPocketComponent.cpp`
6. `Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp`
7. `Source/SingularisInventory/Private/Subsystems/SingularisInventoryItemSubsystem.cpp`
8. `Source/SingularisInventory/Private/Configs/SingularisInventorySettings.cpp`

## 伴随修正

`SpawnItemInWorld` 的 `GetWorld()` 判空:当前代码在判空前直接解引用(潜在崩溃),改为 `ensureMsgf` + return,将未定义行为转为受控失败路径。

## 验证方式

1. 编译(可用 Live Coding 热重载)
2. Output Log 按 `LogSingularisInventory` 过滤观察输出
3. `-LogCmds="LogSingularisInventory Verbose"` 观察正常操作全流程
4. 人为触发失败场景:
   - 空数据表下调用 SpawnItemInWorld → 应见缺行 Warning
   - 移除默认 WBP → 应见构造期 Error
   - 传入空物品 → 应见空入参 Warning

## 取舍记录

- **缺行 Warning 可能刷屏**:配置正确时不产生;配置坏时刷屏本身是"修我"信号,且 UE Output Log 折叠重复消息——接受。
- **check 级别当前零使用**:现有代码的所有失败均可安全中断,无"违反必崩"的内部不变量;级别保留给未来代码。
- **编辑器工厂日志**:本期不插桩(范围聚焦运行时模块;工厂瑕疵已有独立跟踪)。
- **分级约定落点**:本设计文档即约定载体;将来撰写 README 时迁移。
