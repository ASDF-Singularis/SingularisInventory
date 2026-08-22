#pragma once

#include <CoreMinimal.h>
#include <Blueprint/UserWidget.h>

#include "Objects/SingularisItem.h"
#include "SingularisPocketWidget.generated.h"

/**
 * 引力奇点口袋控件。
 *
 * 抽象基类，框架（USingularisPocketWidgetComponent）通过 SPI 推送口袋状态与数据。
 * 用户在蓝图或 C++ 子类中重写 SPI，更新具体控件实现，UI 设计与框架完全解耦。
 */
UCLASS(Abstract, Blueprintable)
class SINGULARISINVENTORY_API USingularisPocketWidget : public UUserWidget
{
	GENERATED_BODY()

public:
#pragma region SPI

	/**
	 * 口袋整体刷新：容量、各插槽物品、当前选中索引。
	 * 由 WidgetComponent 在初始化时主动拉取一次调用，消除错过事件导致的空白期。
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋控件|SPI",
		meta = (DisplayName = "口袋刷新")
	)
	void OnPocketRefresh(int32 Capacity, const TArray<USingularisItem*>& Items, int32 SelectedSlotIndex);

	/** 物品加入指定插槽。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋控件|SPI",
		meta = (DisplayName = "物品加入")
	)
	void OnItemAdded(int32 SlotIndex, USingularisItem* Item);

	/** 物品从指定插槽移除。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋控件|SPI",
		meta = (DisplayName = "物品移除")
	)
	void OnItemRemoved(int32 SlotIndex, USingularisItem* Item);

	/** 选中插槽变化。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋控件|SPI",
		meta = (DisplayName = "选中变化")
	)
	void OnSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex);

#pragma endregion
};
