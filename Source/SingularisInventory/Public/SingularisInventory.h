#pragma once

#include <Modules/ModuleManager.h>

class FSingularisInventoryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
