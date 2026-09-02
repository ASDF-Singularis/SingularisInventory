#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

DECLARE_LOG_CATEGORY_EXTERN(LogSingularisInventoryGameplay, Log, All);

class FSingularisInventoryGameplayModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
