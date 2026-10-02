#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

DECLARE_LOG_CATEGORY_EXTERN(LogSingularisInventory, Log, All);

/**
 * 引力奇点库存运行时模块。
 *
 * 启动时将插件 Shaders 目录映射至虚拟路径 /Plugin/SingularisInventory，
 * 供材质表达式与着色器资产解析自定义 .usf。
 */
class FSingularisInventoryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
