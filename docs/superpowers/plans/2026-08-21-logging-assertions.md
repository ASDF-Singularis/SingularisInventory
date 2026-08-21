# SingularisInventory 日志与断言插桩 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为插件全部 ~20 处静默失败路径与正常操作路径添加日志与断言,使失败可见、可定位、可追踪。

**Architecture:** 单一日志分类 `LogSingularisInventory`(声明于模块头,编辑器模块复用);按四级策略插桩——check 预留(本期零使用)、`ensureMsgf`(低频严重失败,含调用栈)、`UE_LOG Warning`(调用方误用与配置缺失)、`UE_LOG Verbose`(正常操作流)。全部使用 UE 内建宏,不新增封装层。

**Tech Stack:** Unreal Engine 5.8(引擎位于 `C:\Program Files\Epic Games\UE_5.8`),纯 C++ 运行时模块插桩,无新文件类型。

**Spec:** [docs/superpowers/specs/2026-08-21-logging-assertions-design.md](../specs/2026-08-21-logging-assertions-design.md)

## 与规格的偏差说明

1. 规格中"ensureMsgf 本身就是 Error 级日志输出(含分类)"经源码验证(`UE_5.8/Engine/Source/Runtime/Core/Public/Misc/AssertionMacros.h:462` 与 `Private/Misc/AssertionMacros.cpp:970-989`)需修正:**UE 5.8 的 `ensureMsgf` 不接受日志分类参数,输出固定到 LogOutputDevice 分类**(始终可见、含调用栈与消息文本)。本计划按此实现:ensureMsgf 不再另写 UE_LOG 的意图不变,只是输出分类为 LogOutputDevice 而非 LogSingularisInventory。另注意 ensureMsgf 每处站点每次会话仅首次触发(引擎内建去重),属预期行为。
2. 规格映射表将 `SelectSlot` 相同索引归为 Warning,但与其自身判定规则"正常玩法条件 → Verbose"冲突(重复选中同一插槽是无害的幂等调用,UI 中高频)。本计划按判定规则将其归为 **Verbose**(Task 4 Step 7)。实施完成后建议同步修正规格文档对应行。

## Global Constraints

- UE 5.8;编译命令:`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SingularisInventoryEditor Win64 Development -Project="D:\UnrealProjects\VehicleTour\VehicleTour.uproject" -WaitMutex`
- 若编辑器正在运行会锁定 DLL,UBT 编译前先关闭编辑器;或改用编辑器内 Live Coding(Ctrl+Alt+F11)
- 日志分类唯一:`LogSingularisInventory`(默认 `Log`,上限 `All`),声明于 `Source/SingularisInventory/Public/SingularisInventory.h`
- 消息一律中文,格式 `[所有者名] 动作:细节`,上下文用 `GetNameSafe`(空对象输出 "None");`TSubclassOf` 参数必须写 `.Get()`
- `ensureMsgf` 的格式串必须是编译期字面量 `TEXT(...)`;ensure 不用在构造函数内
- OnRep 路径不插桩;查询层(子系统)空入参不记日志
- 工作区有一个与本任务无关的未提交资产(`Content/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget.uasset`),**任何任务提交只 add 本任务文件,不得 `git add -A`**
- 每次提交信息附 Happy/Claude co-author 尾注(模板见 Task 1 的提交步骤)

---

### Task 1: 日志分类声明与定义

**Files:**
- Modify: `Source/SingularisInventory/Public/SingularisInventory.h`
- Modify: `Source/SingularisInventory/Private/SingularisInventory.cpp`

**Interfaces:**
- Produces: `LogSingularisInventory` 日志分类——任何包含 `SingularisInventory.h` 的翻译单元可直接用于 `UE_LOG(LogSingularisInventory, ...)`。Task 2-6 全部依赖。

- [ ] **Step 1: 头文件添加分类声明**

将 `Source/SingularisInventory/Public/SingularisInventory.h` 全文替换为:

```cpp
#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

DECLARE_LOG_CATEGORY_EXTERN(LogSingularisInventory, Log, All);

class FSingularisInventoryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
```

- [ ] **Step 2: cpp 添加分类定义**

将 `Source/SingularisInventory/Private/SingularisInventory.cpp` 前 5 行:

```cpp
#include "SingularisInventory.h"

#include <Interfaces/IPluginManager.h>

#define LOCTEXT_NAMESPACE "FSingularisInventoryModule"
```

替换为(在 include 与 LOCTEXT_NAMESPACE 之间插入定义):

```cpp
#include "SingularisInventory.h"

#include <Interfaces/IPluginManager.h>

DEFINE_LOG_CATEGORY(LogSingularisInventory);

#define LOCTEXT_NAMESPACE "FSingularisInventoryModule"
```

- [ ] **Step 3: 编译验证**

先关闭正在运行的编辑器,然后运行:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SingularisInventoryEditor Win64 Development -Project="D:\UnrealProjects\VehicleTour\VehicleTour.uproject" -WaitMutex
```

Expected: 输出以 `Result: Succeeded` 结尾,无 error。

- [ ] **Step 4: 提交**

```bash
git add Source/SingularisInventory/Public/SingularisInventory.h Source/SingularisInventory/Private/SingularisInventory.cpp
git commit -m "feat: 声明与定义日志分类 LogSingularisInventory

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

---

### Task 2: 插桩 USingularisInventoryComponent

**Files:**
- Modify: `Source/SingularisInventory/Private/Components/SingularisInventoryComponent.cpp`

**Interfaces:**
- Consumes: `LogSingularisInventory`(Task 1)
- Produces: `SpawnItemInWorld` / `CollectItem` 全部失败与成功路径的日志输出(级别见表内注释)

- [ ] **Step 1: 添加分类头文件引用**

将 cpp 头部的插件头引用组:

```cpp
#include "Components/SingularisItemComponent.h"
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "Subsystems/SingularisInventoryItemSubsystem.h"
```

替换为(按字母序插入):

```cpp
#include "Components/SingularisItemComponent.h"
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "SingularisInventory.h"
#include "Subsystems/SingularisInventoryItemSubsystem.h"
```

- [ ] **Step 2: 替换 SpawnItemInWorld 实现**

将 `USingularisInventoryComponent::SpawnItemInWorld` 函数体整体替换为:

```cpp
AActor* USingularisInventoryComponent::SpawnItemInWorld(USingularisItem* Item, FTransform Transform)
{
	// 1) 零信任校验：物品实例必须有效
	if (!IsValid(Item))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：物品实例为空"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 世界必须有效
	UWorld* World = GetWorld();
	if (!ensureMsgf(IsValid(World), TEXT("[%s] SpawnItemInWorld：World 无效"), *GetNameSafe(GetOwner())))
		return nullptr;

	// 3) 经全局查询子系统取物品形态 Actor 类
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!ensureMsgf(IsValid(GameInstance), TEXT("[%s] SpawnItemInWorld：GameInstance 无效"), *GetNameSafe(GetOwner())))
		return nullptr;
	const USingularisInventoryItemSubsystem* ItemSubsystem = GameInstance->GetSubsystem<
		USingularisInventoryItemSubsystem>();
	if (!ensureMsgf(IsValid(ItemSubsystem), TEXT("[%s] SpawnItemInWorld：物品查询子系统无效"), *GetNameSafe(GetOwner())))
		return nullptr;
	const TSubclassOf<AActor> FormActorClass = ItemSubsystem->GetFormActorClass(Item);
	if (!IsValid(FormActorClass))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SpawnItemInWorld：物品类 %s 未配置形态 Actor，请在数据表中补充行"), *GetNameSafe(GetOwner()), *GetNameSafe(Item->GetClass()));
		return nullptr;
	}

	// 4) 生成形态 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* FormActor = World->SpawnActor<AActor>(FormActorClass, Transform, SpawnParams);
	if (!ensureMsgf(IsValid(FormActor), TEXT("[%s] SpawnItemInWorld：生成形态 Actor %s 失败"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActorClass.Get())))
		return nullptr;

	// 5) 查找 ItemComponent；找到则绑定物品实例（可收容），未找到则仅入世不可收容
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (IsValid(ItemComponent))
	{
		ItemComponent->BindItem(Item);
	}
	else
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SpawnItemInWorld：形态 Actor %s 无 ItemComponent，仅入世不可收容"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActor));
	}

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SpawnItemInWorld：物品 %s(%s) 入世成功 → 形态 Actor %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()), *GetNameSafe(FormActor));
	return FormActor;
}
```

说明:原代码第 34 行 `GetWorld()->GetGameInstance()` 与第 48 行 `GetWorld()->SpawnActor` 两次取 World 且未判空;新实现用局部 `World` + ensureMsgf 判空(规格中的伴随修正),行为不变。

- [ ] **Step 3: 替换 CollectItem 实现**

将 `USingularisInventoryComponent::CollectItem` 函数体整体替换为:

```cpp
USingularisItem* USingularisInventoryComponent::CollectItem(
	AActor* FormActor,
	USingularisPocketComponent* TargetContainer
)
{
	// 1) 零信任校验：形态 Actor 必须有效
	if (!IsValid(FormActor))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] CollectItem：形态 Actor 为空"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 查找 ItemComponent；无则无可收容物品
	USingularisItemComponent* ItemComponent = FormActor->FindComponentByClass<USingularisItemComponent>();
	if (!IsValid(ItemComponent))
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：形态 Actor %s 无 ItemComponent，不可收容"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActor));
		return nullptr;
	}

	// 3) 取回物品实例；无物品则不销毁形态 Actor
	USingularisItem* Item = ItemComponent->TakeItem();
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：形态 Actor %s 无物品"), *GetNameSafe(GetOwner()), *GetNameSafe(FormActor));
		return nullptr;
	}

	// 4) 销毁形态 Actor
	FormActor->Destroy();

	// 5) 提供目标容器则尝试入容器；满或未提供容器时返回实例由调用方处置
	if (IsValid(TargetContainer))
	{
		const int32 SlotIndex = TargetContainer->AddItem(Item);
		if (SlotIndex != INDEX_NONE)
		{
			UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：物品 %s(%s) 收回成功，入容器 %s 插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()), *GetNameSafe(TargetContainer->GetOwner()), SlotIndex);
		}
		else
		{
			UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：物品 %s(%s) 收回成功，容器已满未放入"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()));
		}
	}
	else
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CollectItem：物品 %s(%s) 收回成功，未提供容器"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()));
	}

	return Item;
}
```

- [ ] **Step 4: 编译验证**

同 Task 1 Step 3 的编译命令。Expected: `Result: Succeeded`。

- [ ] **Step 5: 提交**

```bash
git add Source/SingularisInventory/Private/Components/SingularisInventoryComponent.cpp
git commit -m "feat: 为库存组件添加日志与断言插桩

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

---

### Task 3: 插桩 USingularisItemComponent

**Files:**
- Modify: `Source/SingularisInventory/Private/Components/SingularisItemComponent.cpp`

**Interfaces:**
- Consumes: `LogSingularisInventory`(Task 1)
- Produces: `BindItem` / `TakeItem` 的 Warning/Verbose 日志

- [ ] **Step 1: 添加分类头文件引用**

将 cpp 头部的插件头引用组:

```cpp
#include "Objects/SingularisItem.h"
```

替换为:

```cpp
#include "Objects/SingularisItem.h"
#include "SingularisInventory.h"
```

- [ ] **Step 2: 替换 BindItem 实现**

将 `USingularisItemComponent::BindItem` 函数体整体替换为:

```cpp
void USingularisItemComponent::BindItem(USingularisItem* InItem)
{
	// 1) 零信任校验：空入参直接忽略
	if (InItem == nullptr)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] BindItem：物品实例为空"), *GetNameSafe(GetOwner()));
		return;
	}

	// 2) 幂等：已持有同一实例则无副作用
	if (Item == InItem)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] BindItem：物品 %s 已持有，忽略"), *GetNameSafe(GetOwner()), *GetNameSafe(InItem));
		return;
	}

	// 3) 若已持有其他实例，先解除旧引用并广播取出，保证单一持有
	if (Item != nullptr)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] BindItem：替换旧物品 %s → %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Item.Get()), *GetNameSafe(InItem));
		UnregisterItemSubObject();
		OnItemReleasedEvent.Broadcast(Item.Get());
		Item = nullptr;
	}

	// 4) 建立新的持有关系，注册复制子对象并广播移入
	Item = InItem;
	RegisterItemSubObject();
	OnItemBoundEvent.Broadcast(InItem);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] BindItem：物品 %s(%s) 绑定成功"), *GetNameSafe(GetOwner()), *GetNameSafe(InItem), *GetNameSafe(InItem->GetClass()));
}
```

- [ ] **Step 3: 替换 TakeItem 实现**

将 `USingularisItemComponent::TakeItem` 函数体整体替换为:

```cpp
USingularisItem* USingularisItemComponent::TakeItem()
{
	// 1) 空状态安全返回
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] TakeItem：无物品可取出"), *GetNameSafe(GetOwner()));
		return nullptr;
	}

	// 2) 解除复制注册，广播取出并清空持有，将引用权交还调用方
	USingularisItem* const OutItem = Item.Get();
	UnregisterItemSubObject();
	OnItemReleasedEvent.Broadcast(OutItem);
	Item = nullptr;

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] TakeItem：物品 %s(%s) 取出成功"), *GetNameSafe(GetOwner()), *GetNameSafe(OutItem), *GetNameSafe(OutItem->GetClass()));
	return OutItem;
}
```

- [ ] **Step 4: 编译验证**

同 Task 1 Step 3 的编译命令。Expected: `Result: Succeeded`。

- [ ] **Step 5: 提交**

```bash
git add Source/SingularisInventory/Private/Components/SingularisItemComponent.cpp
git commit -m "feat: 为物品组件添加日志与断言插桩

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

---

### Task 4: 插桩 USingularisPocketComponent

**Files:**
- Modify: `Source/SingularisInventory/Private/Components/SingularisPocketComponent.cpp`

**Interfaces:**
- Consumes: `LogSingularisInventory`(Task 1)
- Produces: 口袋插槽 API 全部 Warning/Verbose 日志与 Slots/Capacity 一致性 ensure

- [ ] **Step 1: 添加分类头文件引用**

将 cpp 头部的插件头引用组:

```cpp
#include "Objects/SingularisItem.h"
```

替换为:

```cpp
#include "Objects/SingularisItem.h"
#include "SingularisInventory.h"
```

- [ ] **Step 2: BeginPlay 添加初始化 Verbose**

将 BeginPlay 函数体:

```cpp
void USingularisPocketComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1) 仅权威端预分配插槽数组；客户端由复制同步，避免本地写入复制属性
	if (GetOwner()->HasAuthority())
		InitializeSlots();

	// 2) 建立客户端 OnRep diff 的初始基线快照
	PreviousSlotsSnapshot = Slots;
	PreviousSelectedSlotIndex = SelectedSlotIndex;
}
```

替换为:

```cpp
void USingularisPocketComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1) 仅权威端预分配插槽数组；客户端由复制同步，避免本地写入复制属性
	if (GetOwner()->HasAuthority())
	{
		InitializeSlots();
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] 口袋插槽初始化完成，容量 %d"), *GetNameSafe(GetOwner()), Capacity);
	}

	// 2) 建立客户端 OnRep diff 的初始基线快照
	PreviousSlotsSnapshot = Slots;
	PreviousSelectedSlotIndex = SelectedSlotIndex;
}
```

- [ ] **Step 3: 替换 AddItem 实现**

将 `USingularisPocketComponent::AddItem` 函数体整体替换为:

```cpp
int32 USingularisPocketComponent::AddItem(USingularisItem* Item)
{
	// 1) 零信任校验：空入参直接忽略
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] AddItem：物品实例为空"), *GetNameSafe(GetOwner()));
		return INDEX_NONE;
	}

	// 2) 插槽与容量一致性校验：运行时修改 Capacity 需重新初始化（仅权威端校验）
	if (GetOwner()->HasAuthority() && !ensureMsgf(Slots.Num() == Capacity, TEXT("[%s] AddItem：插槽数 %d 与容量 %d 失配"), *GetNameSafe(GetOwner()), Slots.Num(), Capacity))
		return INDEX_NONE;

	// 3) 幂等：物品已存在于此口袋，直接返回其所在插槽
	const int32 ExistingSlot = FindSlotOfItem(Item);
	if (ExistingSlot != INDEX_NONE)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] AddItem：物品 %s 已在插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), ExistingSlot);
		return ExistingSlot;
	}

	// 4) 寻找首个空插槽放入，注册复制子对象并广播原子过渡
	const int32 TargetSlot = FindFirstEmptySlot();
	if (TargetSlot == INDEX_NONE)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] AddItem：口袋已满，物品 %s 未放入"), *GetNameSafe(GetOwner()), *GetNameSafe(Item));
		return INDEX_NONE;
	}

	Slots[TargetSlot].Item = Item;
	RegisterSlotSubObject(TargetSlot);
	BroadcastSlotTransition(TargetSlot, nullptr, Item);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] AddItem：物品 %s(%s) 放入插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()), TargetSlot);
	return TargetSlot;
}
```

- [ ] **Step 4: 替换 AddItemAt 实现**

将 `USingularisPocketComponent::AddItemAt` 函数体整体替换为:

```cpp
bool USingularisPocketComponent::AddItemAt(USingularisItem* Item, const int32 SlotIndex)
{
	// 1) 零信任校验：空入参或非法索引直接失败
	if (Item == nullptr || !IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] AddItemAt：入参非法（物品 %s，索引 %d）"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), SlotIndex);
		return false;
	}

	// 2) 目标插槽必须为空，且物品未存在于其他插槽
	if (!Slots[SlotIndex].IsEmpty())
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] AddItemAt：插槽 %d 已被占用"), *GetNameSafe(GetOwner()), SlotIndex);
		return false;
	}
	if (FindSlotOfItem(Item) != INDEX_NONE)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] AddItemAt：物品 %s 已在插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), FindSlotOfItem(Item));
		return false;
	}

	Slots[SlotIndex].Item = Item;
	RegisterSlotSubObject(SlotIndex);
	BroadcastSlotTransition(SlotIndex, nullptr, Item);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] AddItemAt：物品 %s(%s) 放入插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(Item), *GetNameSafe(Item->GetClass()), SlotIndex);
	return true;
}
```

- [ ] **Step 5: 替换 RemoveItem 实现**

将 `USingularisPocketComponent::RemoveItem` 函数体整体替换为:

```cpp
bool USingularisPocketComponent::RemoveItem(USingularisItem* Item)
{
	if (Item == nullptr)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] RemoveItem：物品实例为空"), *GetNameSafe(GetOwner()));
		return false;
	}

	const int32 TargetSlot = FindSlotOfItem(Item);
	if (TargetSlot == INDEX_NONE)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] RemoveItem：物品 %s 不在口袋中"), *GetNameSafe(GetOwner()), *GetNameSafe(Item));
		return false;
	}

	return RemoveItemAt(TargetSlot) != nullptr;
}
```

- [ ] **Step 6: 替换 RemoveItemAt 实现**

将 `USingularisPocketComponent::RemoveItemAt` 函数体整体替换为:

```cpp
USingularisItem* USingularisPocketComponent::RemoveItemAt(const int32 SlotIndex)
{
	// 1) 索引合法性校验
	if (!IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] RemoveItemAt：索引 %d 非法"), *GetNameSafe(GetOwner()), SlotIndex);
		return nullptr;
	}

	// 2) 空插槽安全返回
	if (Slots[SlotIndex].IsEmpty())
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] RemoveItemAt：插槽 %d 为空"), *GetNameSafe(GetOwner()), SlotIndex);
		return nullptr;
	}

	// 3) 解除复制注册，清空插槽，广播原子过渡，返还物品实例
	USingularisItem* const OldItem = Slots[SlotIndex].Item;
	UnregisterSlotSubObject(SlotIndex);
	Slots[SlotIndex].Item = nullptr;
	BroadcastSlotTransition(SlotIndex, OldItem, nullptr);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] RemoveItemAt：物品 %s(%s) 移出插槽 %d"), *GetNameSafe(GetOwner()), *GetNameSafe(OldItem), *GetNameSafe(OldItem->GetClass()), SlotIndex);
	return OldItem;
}
```

- [ ] **Step 7: 替换 SelectSlot / SelectNext / SelectPrevious 实现**

将 `SelectSlot` 函数体整体替换为:

```cpp
void USingularisPocketComponent::SelectSlot(const int32 SlotIndex)
{
	// 1) 合法值：INDEX_NONE 清空选中，或有效插槽索引；其余忽略
	if (SlotIndex != INDEX_NONE && !IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SelectSlot：索引 %d 非法"), *GetNameSafe(GetOwner()), SlotIndex);
		return;
	}

	// 2) 幂等：与当前选中相同则无副作用
	if (SlotIndex == SelectedSlotIndex)
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SelectSlot：选中未变化（%d）"), *GetNameSafe(GetOwner()), SlotIndex);
		return;
	}

	const int32 OldSlotIndex = SelectedSlotIndex;
	SelectedSlotIndex = SlotIndex;
	OnSelectionChangedEvent.Broadcast(OldSlotIndex, SlotIndex);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SelectSlot：选中 %d → %d"), *GetNameSafe(GetOwner()), OldSlotIndex, SlotIndex);
}
```

将 `SelectNext` 函数体整体替换为:

```cpp
void USingularisPocketComponent::SelectNext()
{
	if (Capacity <= 0)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SelectNext：容量 %d 无效"), *GetNameSafe(GetOwner()), Capacity);
		return;
	}

	const int32 Base = SelectedSlotIndex == INDEX_NONE ? 0 : SelectedSlotIndex;
	const int32 Target = (Base + 1) % Capacity;
	SelectSlot(Target);
}
```

将 `SelectPrevious` 函数体整体替换为:

```cpp
void USingularisPocketComponent::SelectPrevious()
{
	if (Capacity <= 0)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SelectPrevious：容量 %d 无效"), *GetNameSafe(GetOwner()), Capacity);
		return;
	}

	const int32 Base = SelectedSlotIndex == INDEX_NONE ? Capacity - 1 : SelectedSlotIndex;
	const int32 Target = (Base - 1 + Capacity) % Capacity;
	SelectSlot(Target);
}
```

- [ ] **Step 8: 替换 SwapSlots 实现**

将 `USingularisPocketComponent::SwapSlots` 函数体整体替换为:

```cpp
void USingularisPocketComponent::SwapSlots(const int32 SlotIndexA, const int32 SlotIndexB)
{
	// 1) 两端均合法且不同才交换
	if (!IsValidSlotIndex(SlotIndexA) || !IsValidSlotIndex(SlotIndexB) || SlotIndexA == SlotIndexB)
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] SwapSlots：索引非法（%d ↔ %d）"), *GetNameSafe(GetOwner()), SlotIndexA, SlotIndexB);
		return;
	}

	// 2) 交换物品引用，物品仍在口袋内无需调整复制子对象注册
	USingularisItem* const OldA = Slots[SlotIndexA].Item;
	USingularisItem* const OldB = Slots[SlotIndexB].Item;
	Slots[SlotIndexA].Item = OldB;
	Slots[SlotIndexB].Item = OldA;

	// 3) 逐插槽广播原子过渡
	BroadcastSlotTransition(SlotIndexA, OldA, OldB);
	BroadcastSlotTransition(SlotIndexB, OldB, OldA);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] SwapSlots：插槽 %d ↔ %d 交换完成"), *GetNameSafe(GetOwner()), SlotIndexA, SlotIndexB);
}
```

- [ ] **Step 9: 替换 Clear 实现**

将 `USingularisPocketComponent::Clear` 函数体整体替换为:

```cpp
void USingularisPocketComponent::Clear()
{
	int32 ClearedCount = 0;

	for (auto i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].IsEmpty())
			continue;

		USingularisItem* const OldItem = Slots[i].Item;
		UnregisterSlotSubObject(i);
		Slots[i].Item = nullptr;
		BroadcastSlotTransition(i, OldItem, nullptr);
		++ClearedCount;
	}

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] Clear：清空 %d 个插槽"), *GetNameSafe(GetOwner()), ClearedCount);
}
```

- [ ] **Step 10: 编译验证**

同 Task 1 Step 3 的编译命令。Expected: `Result: Succeeded`。

- [ ] **Step 11: 提交**

```bash
git add Source/SingularisInventory/Private/Components/SingularisPocketComponent.cpp
git commit -m "feat: 为口袋组件添加日志与断言插桩

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

---

### Task 5: 插桩 USingularisPocketWidgetComponent

**Files:**
- Modify: `Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp`

**Interfaces:**
- Consumes: `LogSingularisInventory`(Task 1)
- Produces: 口袋控件组件的 Error/Warning/ensure/Verbose 日志

- [ ] **Step 1: 添加分类头文件引用**

将 cpp 头部的插件头引用组:

```cpp
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "Widgets/SingularisPocketWidget.h"
```

替换为:

```cpp
#include "Components/SingularisPocketComponent.h"
#include "Objects/SingularisItem.h"
#include "SingularisInventory.h"
#include "Widgets/SingularisPocketWidget.h"
```

- [ ] **Step 2: 构造函数添加加载失败 Error 日志**

将构造函数的 FClassFinder 处理段:

```cpp
	if (WidgetClassFinder.Succeeded())
		PocketWidgetClass = WidgetClassFinder.Class;
```

替换为:

```cpp
	if (WidgetClassFinder.Succeeded())
	{
		PocketWidgetClass = WidgetClassFinder.Class;
	}
	else
	{
		UE_LOG(LogSingularisInventory, Error, TEXT("默认口袋控件加载失败：%s"), TEXT("/SingularisInventory/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget"));
	}
```

- [ ] **Step 3: ResolveOwningLocalPlayerController 添加 Verbose**

将:

```cpp
	// 2) 仅本客户端拥有的本地控制器才有效，避免为其他玩家复制的 Pawn 创建幽灵控件
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		return nullptr;
```

替换为:

```cpp
	// 2) 仅本客户端拥有的本地控制器才有效，避免为其他玩家复制的 Pawn 创建幽灵控件
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] ResolveOwningLocalPlayerController：非本地控制者，跳过 UI"), *GetNameSafe(GetOwner()));
		return nullptr;
	}
```

- [ ] **Step 4: 替换 CreatePocketWidget 实现**

将 `USingularisPocketWidgetComponent::CreatePocketWidget` 函数体整体替换为:

```cpp
void USingularisPocketWidgetComponent::CreatePocketWidget()
{
	// 1) 仅本客户端拥有的 Actor（Pawn 由本地 PC 控制，或 Owner 本身即本地 PC）才创建 UI，
	//    避免为其他玩家复制的 Pawn 创建幽灵控件
	APlayerController* PlayerController = ResolveOwningLocalPlayerController();
	if (!IsValid(PlayerController))
		return; // 非本地控制者，ResolveOwningLocalPlayerController 已记录 Verbose

	// 2) 零信任校验：未配置控件类则跳过
	if (!IsValid(PocketWidgetClass))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] CreatePocketWidget：未配置控件类"), *GetNameSafe(GetOwner()));
		return;
	}

	PocketWidget = CreateWidget<USingularisPocketWidget>(PlayerController, PocketWidgetClass);
	if (!ensureMsgf(IsValid(PocketWidget), TEXT("[%s] CreatePocketWidget：创建控件 %s 失败"), *GetNameSafe(GetOwner()), *GetNameSafe(PocketWidgetClass.Get())))
		return;

	PocketWidget->AddToViewport();

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] CreatePocketWidget：控件 %s 创建成功并加入视口"), *GetNameSafe(GetOwner()), *GetNameSafe(PocketWidgetClass.Get()));
}
```

- [ ] **Step 5: ObservePocketComponent 添加解析失败 Warning 与成功 Verbose**

将 `USingularisPocketWidgetComponent::ObservePocketComponent` 函数体整体替换为:

```cpp
void USingularisPocketWidgetComponent::ObservePocketComponent()
{
	const APlayerController* PlayerController = ResolveOwningLocalPlayerController();
	if (!IsValid(PlayerController))
		return;

	// 1) 零信任校验：控件与目标组件均需有效
	if (!IsValid(PocketWidget))
		return; // 创建失败时 CreatePocketWidget 已记录日志

	USingularisPocketComponent* PocketComponent = Cast<USingularisPocketComponent>(
		PocketComponentReference.GetComponent(GetOwner())
	);
	if (!IsValid(PocketComponent))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("[%s] ObservePocketComponent：未解析到口袋组件，请检查 PocketComponentReference 配置"), *GetNameSafe(GetOwner()));
		return;
	}

	// 2) 绑定事件实现事件驱动观察者模式
	PocketComponent->OnItemAddedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemAdded);
	PocketComponent->OnItemRemovedEvent.AddDynamic(this, &USingularisPocketWidgetComponent::HandleItemRemoved);
	PocketComponent->OnSelectionChangedEvent.AddDynamic(
		this,
		&USingularisPocketWidgetComponent::HandleSelectionChanged
	);

	// 3) 主动拉取一次全量状态，消除错过事件导致的空白期
	RefreshPocket(PocketComponent);

	UE_LOG(LogSingularisInventory, Verbose, TEXT("[%s] ObservePocketComponent：已绑定 %s 事件并完成全量拉取"), *GetNameSafe(GetOwner()), *GetNameSafe(PocketComponent));
}
```

- [ ] **Step 6: 编译验证**

同 Task 1 Step 3 的编译命令。Expected: `Result: Succeeded`。

- [ ] **Step 7: 提交**

```bash
git add Source/SingularisInventory/Private/Components/SingularisPocketWidgetComponent.cpp
git commit -m "feat: 为口袋控件组件添加日志与断言插桩

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

---

### Task 6: 插桩 USingularisInventoryItemSubsystem 与 USingularisInventorySettings

**Files:**
- Modify: `Source/SingularisInventory/Private/Subsystems/SingularisInventoryItemSubsystem.cpp`
- Modify: `Source/SingularisInventory/Private/Configs/SingularisInventorySettings.cpp`

**Interfaces:**
- Consumes: `LogSingularisInventory`(Task 1)
- Produces: 查询层与设置层的 Warning/Error 日志(根因层,与 Task 2 组件层日志互补)

- [ ] **Step 1: 子系统 cpp 添加分类头文件引用**

将 `Source/SingularisInventory/Private/Subsystems/SingularisInventoryItemSubsystem.cpp` 头部的插件头引用组:

```cpp
#include "Configs/SingularisInventorySettings.h"
#include "Objects/SingularisItem.h"
```

替换为:

```cpp
#include "Configs/SingularisInventorySettings.h"
#include "Objects/SingularisItem.h"
#include "SingularisInventory.h"
```

- [ ] **Step 2: 替换子系统 GetItemTable 实现**

将 `USingularisInventoryItemSubsystem::GetItemTable` 函数体整体替换为:

```cpp
UDataTable* USingularisInventoryItemSubsystem::GetItemTable() const
{
	const USingularisInventorySettings* Settings = GetDefault<USingularisInventorySettings>();
	if (!IsValid(Settings) || !IsValid(Settings->ItemTable.Get()))
	{
		UE_LOG(LogSingularisInventory, Warning, TEXT("物品数据表无效，请在项目设置「Singularis → Singularis Inventory」中配置 ItemTable"));
		return nullptr;
	}
	return Settings->ItemTable.Get();
}
```

- [ ] **Step 3: 替换子系统 FindItemRowByClass 实现**

将 `USingularisInventoryItemSubsystem::FindItemRowByClass` 函数体整体替换为:

```cpp
const FSingularisItemRow* USingularisInventoryItemSubsystem::FindItemRowByClass(
	TSubclassOf<USingularisItem> ItemClass
) const
{
	UDataTable* ItemTable = GetItemTable();
	if (!IsValid(ItemTable) || !IsValid(ItemClass.Get()))
		return nullptr; // 表无效时 GetItemTable 已记录日志；空入参不记

	const UClass* ItemClassPtr = ItemClass.Get();
	for (const auto& Pair : ItemTable->GetRowMap())
	{
		const auto Row = reinterpret_cast<const FSingularisItemRow*>(Pair.Value);
		if (IsValid(Row->ItemClass) && Row->ItemClass.Get() == ItemClassPtr)
			return Row;
	}

	UE_LOG(LogSingularisInventory, Warning, TEXT("物品类 %s 未在数据表中找到行，请检查物品数据配置"), *GetNameSafe(ItemClass.Get()));
	return nullptr;
}
```

- [ ] **Step 4: 设置 cpp 添加分类头文件引用**

将 `Source/SingularisInventory/Private/Configs/SingularisInventorySettings.cpp` 头部:

```cpp
#include "Configs/SingularisInventorySettings.h"

#include <UObject/ConstructorHelpers.h>
```

替换为:

```cpp
#include "Configs/SingularisInventorySettings.h"

#include <UObject/ConstructorHelpers.h>

#include "SingularisInventory.h"
```

- [ ] **Step 5: 设置构造函数添加加载失败 Error 日志**

将构造函数的 FObjectFinder 处理段:

```cpp
	if (ItemTableFinder.Succeeded())
		ItemTable = ItemTableFinder.Object;
```

替换为:

```cpp
	if (ItemTableFinder.Succeeded())
	{
		ItemTable = ItemTableFinder.Object;
	}
	else
	{
		UE_LOG(LogSingularisInventory, Error, TEXT("默认物品数据表加载失败：%s"), TEXT("/SingularisInventory/DataTables/DT_SingularisInventory_ItemTable"));
	}
```

- [ ] **Step 6: 编译验证**

同 Task 1 Step 3 的编译命令。Expected: `Result: Succeeded`。

- [ ] **Step 7: 提交**

```bash
git add Source/SingularisInventory/Private/Subsystems/SingularisInventoryItemSubsystem.cpp Source/SingularisInventory/Private/Configs/SingularisInventorySettings.cpp
git commit -m "feat: 为物品子系统与项目设置添加日志与断言插桩

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

---

### Task 7: 整体运行时验证

**Files:**
- 无新增/修改(发现缺口则回对应任务修补)

**Interfaces:**
- Consumes: Task 1-6 全部插桩产物

- [ ] **Step 1: 全量编译**

关闭编辑器,运行 Task 1 Step 3 的编译命令。Expected: `Result: Succeeded`。

- [ ] **Step 2: 启动编辑器做基础观察**

打开 `D:\UnrealProjects\VehicleTour\VehicleTour.uproject`。打开 Window → Developer Tools → Output Log,搜索框输入 `LogSingularisInventory`,确认:
- 启动期无 Error/Warning(默认 WBP 与数据表存在时)
- 在 PIE 中运行含口袋组件的 Pawn:Output Log 中勾选 `Verbose` 显示级别后,可见 `口袋插槽初始化完成，容量 N` 的 Verbose 日志

- [ ] **Step 3: 触发失败场景验证表**

在 PIE(单机即可,HasAuthority 恒真)中逐项触发,在 Output Log 中核对输出:

| # | 触发方式 | 预期输出 |
|---|---|---|
| 1 | BP 中对空物品调用 `AddItem` / `BindItem` / `SpawnItemInWorld` / `RemoveItem` | Warning:对应 `物品实例为空` |
| 2 | 空数据表(默认状态)下对有效物品调用 `SpawnItemInWorld` | Warning ×2:`物品类 X 未在数据表中找到行` + `未配置形态 Actor，请在数据表中补充行` |
| 3 | `AddItemAt` 指定已占用插槽 | Warning:`插槽 N 已被占用` |
| 4 | `AddItemAt` 指定非法索引(-1 / 99) | Warning:`入参非法` |
| 5 | `SelectSlot(99)` | Warning:`索引 99 非法` |
| 6 | 已满口袋继续 `AddItem` | Verbose:`口袋已满`(需开启 Verbose 显示) |
| 7 | `RemoveItemAt` 空插槽 | Verbose:`插槽 N 为空` |
| 8 | 完整流程:生成物品 → 收容 → 入口袋 → 选中 → 移出 | 一串 Verbose 成功日志,可完整还原操作序列 |

- [ ] **Step 4: 触发构造期 Error 场景**

临时将 `Content/UserInterfaces/WBP_SingularisInventory_SingularisPocketWidget.uasset` 重命名(如加 `_bak` 后缀),重启编辑器,Expected:Output Log 中出现 Error:`默认口袋控件加载失败`。验证后恢复原文件名。**注意:该资产工作区已有未提交修改,重命名验证后必须恢复原状,不得提交该资产的任何变更。**

- [ ] **Step 5: 命令行 Verbose 全流程**

用参数启动:`"D:\UnrealProjects\VehicleTour\VehicleTour.uproject" -LogCmds="LogSingularisInventory Verbose"`,在 PIE 中走一遍完整流程,确认 Verbose 流从初始化到每次增删改换全覆盖。

- [ ] **Step 6: 提交收尾**

若验证发现问题并修复,按对应任务的文件范围提交修复:

```bash
git add <修复的文件路径>
git commit -m "fix: 修正日志插桩验证发现的问题

Generated with [Claude Code](https://claude.ai/code)
via [Happy](https://happy.engineering)

Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Happy <yesreply@happy.engineering>"
```

无修复则无需提交,任务完成。
