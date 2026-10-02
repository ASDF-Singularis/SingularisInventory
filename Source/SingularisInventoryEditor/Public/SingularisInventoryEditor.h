#pragma once

#include <CoreMinimal.h>
#include <IAssetTypeActions.h>
#include <Modules/ModuleManager.h>

class IAssetTools;

DECLARE_LOG_CATEGORY_EXTERN(LogSingularisInventoryEditor, Log, All);

/**
 * 引力奇点库存编辑器模块。
 *
 * 向资产工具注册"Singularis"资产分类，并登记五类资产类型行为
 * （物品定义 / 物品实例 / 物品形态 / 物品片段 / 口袋控件）；卸载时反注册。
 */
class FSingularisInventoryEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 已登记的资产类型行为，供卸载时反注册。 */
	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions{};

	/** 登记资产类型行为并留存引用。 */
	void RegisterAssetTypeAction(IAssetTools& AssetTools, const TSharedRef<IAssetTypeActions>& Action);
};
