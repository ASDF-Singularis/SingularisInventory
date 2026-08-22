#include "SingularisInventory.h"

#include <Interfaces/IPluginManager.h>

DEFINE_LOG_CATEGORY(LogSingularisInventory);

#define LOCTEXT_NAMESPACE "FSingularisInventoryModule"

void FSingularisInventoryModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SingularisInventory"));
	if (!ensureMsgf(
		Plugin.IsValid(),
		TEXT("StartupModule：无法找到插件 SingularisInventory，跳过着色器目录映射")
	))
		return;

	const FString PluginShaderDir = FPaths::Combine(
		Plugin->GetBaseDir(),
		TEXT("Shaders")
	);
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/SingularisInventory"), PluginShaderDir);

	UE_LOG(
		LogSingularisInventory,
		Verbose,
		TEXT("StartupModule：着色器目录映射已注册 -> %s"),
		*PluginShaderDir
	);
}

void FSingularisInventoryModule::ShutdownModule()
{
	UE_LOG(LogSingularisInventory, Verbose, TEXT("ShutdownModule：模块卸载"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSingularisInventoryModule, SingularisInventory)
