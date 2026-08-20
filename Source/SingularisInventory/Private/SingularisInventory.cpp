#include "SingularisInventory.h"

#include <Interfaces/IPluginManager.h>

#define LOCTEXT_NAMESPACE "FSingularisInventoryModule"

void FSingularisInventoryModule::StartupModule()
{
	const FString PluginShaderDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("SingularisInventory"))->GetBaseDir(),
		TEXT("Shaders")
	);
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/SingularisInventory"), PluginShaderDir);
}

void FSingularisInventoryModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSingularisInventoryModule, SingularisInventory)
