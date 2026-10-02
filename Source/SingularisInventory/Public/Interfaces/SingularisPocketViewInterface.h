#pragma once

#include <CoreMinimal.h>
#include <UObject/Interface.h>

#include "SingularisPocketViewInterface.generated.h"

class USingularisItem;

/**
 * 引力奇点口袋视图接口。
 *
 * 口袋视图契约：实现者接收口袋组件的全量刷新与增量事件，由 WidgetComponent 在
 * 绑定完成与视图替换时经 Execute_ 调用。实现者不限于控件——任意 UObject 实现
 * 本接口即可接入驱动。
 */
UINTERFACE(Blueprintable, BlueprintType)
class USingularisPocketViewInterface : public UInterface
{
	GENERATED_BODY()
};

/** 口袋视图实现接口：接收口袋整体的全量刷新与插槽、选中增删事件。 */
class SINGULARISINVENTORY_API ISingularisPocketViewInterface
{
	GENERATED_BODY()

public:
	/**
	 * 口袋整体刷新：容量、各插槽物品、当前选中索引。
	 * 由 WidgetComponent 在绑定完成与视图替换时主动调用，消除错过事件导致的空白期；
	 * 外部注入的视图必须预期本函数在其被设置后立即到达。
	 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "引力奇点口袋视图接口",
		meta = (DisplayName = "口袋刷新")
	)
	void OnPocketRefresh(int32 Capacity, const TArray<USingularisItem*>& Items, int32 SelectedSlotIndex);

	/** 物品加入指定插槽。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "引力奇点口袋视图接口",
		meta = (DisplayName = "物品加入")
	)
	void OnItemAdded(int32 SlotIndex, USingularisItem* Item);

	/** 物品从指定插槽移除。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "引力奇点口袋视图接口",
		meta = (DisplayName = "物品移除")
	)
	void OnItemRemoved(int32 SlotIndex, USingularisItem* Item);

	/** 选中插槽变化。 */
	UFUNCTION(
		BlueprintNativeEvent,
		BlueprintCallable,
		Category = "引力奇点口袋视图接口",
		meta = (DisplayName = "选中变化")
	)
	void OnSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex);
};
