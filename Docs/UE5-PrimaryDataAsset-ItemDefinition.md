# UE5 PrimaryDataAsset 与物品定义

> 探索 **UPrimaryDataAsset** 与物品定义的关系。物品定义是一个概念——物品的静态描述；**UPrimaryDataAsset** 是一个基类——描述被资产化后的承载者。本文考察两者的结合点、能力边界与取舍，不重复引擎文档对两者的定义。

## 概念与基类

- 物品定义描述物品的静态属性：名称、图标、标签、属性、行为。物品定义类通常命名为 `U[项目名]ItemDefinition`。
- 定义的承载形态有三类：**DataTable** 行、**TSubclassOf** 类引用、数据资产（**UDataAsset** / **UPrimaryDataAsset**）。
- 形态决定能力边界。**DataTable** 行只能被整表查询；类引用只能被实例化；数据资产可被内容浏览器创建、随打包、被 **Asset Manager** 管理。
- **UPrimaryDataAsset** 继承自 **UDataAsset**，**UDataAsset** 继承自 **UObject**。以 **UPrimaryDataAsset** 为基类的定义仍然是 **UObject**。"PrimaryDataAsset 还是 UObject"的二选一不成立，实际选择的是定义以何种形态存在。

## 结合点

### 资产身份

定义作为独立资产存在于内容浏览器。创建、复制、重命名、移动在编辑器完成。重命名 / 移动由 **Redirector** 修复引用。

### Asset Manager 集成

**GetPrimaryAssetId()** 返回 **FPrimaryAssetId**（类型 + 名称），定义获得全局唯一 ID。注册 **PrimaryAssetTypesToScan** 后：

- **GetPrimaryAssetIdList** 枚举全部定义，替代表查询
- **TSoftObjectPtr** + **RequestAsyncLoad** 按需异步加载
- 存档 / 网络复制只存 ID，加载时经 **FSoftObjectPath** 解析

### AssetBundleData

定义声明一组随定义一起加载的资产（图标、网格、特效）。运行时一次拉齐，解决"拿到定义但引用资产未加载"。

## 结合产生的边界

### 定义与实例分离

定义是共享资产，不携带动态状态。堆叠数、耐久、冷却属于实例。实例引用定义，实例状态不写回定义。定义被当作实例使用时，所有引用同一定义的实例互相污染。

### 行为扩展放在片段

定义为每种行为新建 **Blueprint** 类会产生继承爆炸。行为放定义内 **Instanced** 数组持有的 **EditInlineNew** **UObject** 子对象（**Fragment**）。定义与 **Fragment** 是包含关系。

### 实例持有定义引用

运行时实例持有 **TObjectPtr** 或 **TSoftObjectPtr** 指向定义。复制与存档同步 ID，不同步资产本体。

## 边界之外

- 定义无需被引用、枚举、异步加载时，**DataTable** 或 **UDataAsset** 足以承载。
- 定义内部的 **Fragment**：纯 **UObject** + **EditInlineNew**。
- 运行时瞬态对象：纯 **UObject**，与资产无关。

## 判定

选择 **UPrimaryDataAsset** 意味着接受三件事：定义资产化、ID 引用、片段扩展。三项同时需要时选择它；不需要时其他形态成立。
