#pragma once

#include <CoreMinimal.h>
#include <Blueprint/UserWidget.h>

#include "Interfaces/SingularisPocketViewInterface.h"
#include "Objects/SingularisItem.h"
#include "SingularisPocketWidget.generated.h"

/**
 * 引力奇点口袋控件。
 *
 * 默认口袋视图：实现 ISingularisPocketViewInterface，框架（USingularisPocketWidgetComponent）
 * 经接口推送口袋状态与数据。用户在蓝图或 C++ 子类中覆写 SPI，更新具体控件实现。
 */
UCLASS(Blueprintable)
class SINGULARISINVENTORY_API USingularisPocketWidget : public UUserWidget, public ISingularisPocketViewInterface
{
	GENERATED_BODY()

public:
#pragma region SPI

	/** 口袋整体刷新：容量、各插槽物品、当前选中索引。 */
	virtual void OnPocketRefresh_Implementation(
		int32 Capacity,
		const TArray<USingularisItem*>& Items,
		int32 SelectedSlotIndex
	) override;

	/** 物品加入指定插槽。 */
	virtual void OnItemAdded_Implementation(int32 SlotIndex, USingularisItem* Item) override;

	/** 物品从指定插槽移除。 */
	virtual void OnItemRemoved_Implementation(int32 SlotIndex, USingularisItem* Item) override;

	/** 选中插槽变化。 */
	virtual void OnSelectionChanged_Implementation(int32 OldSlotIndex, int32 NewSlotIndex) override;

#pragma endregion
};
