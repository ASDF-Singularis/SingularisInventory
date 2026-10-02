#include "Widgets/SingularisPocketWidget.h"

// 基类默认实现为空：框架只保证调用时序与数据到达，实际控件更新由蓝图或 C++ 子类覆写 SPI 完成

void USingularisPocketWidget::OnPocketRefresh_Implementation(
	int32 Capacity,
	const TArray<USingularisItem*>& Items,
	int32 SelectedSlotIndex
) {}

void USingularisPocketWidget::OnItemAdded_Implementation(int32 SlotIndex, USingularisItem* Item) {}

void USingularisPocketWidget::OnItemRemoved_Implementation(int32 SlotIndex, USingularisItem* Item) {}

void USingularisPocketWidget::OnSelectionChanged_Implementation(int32 OldSlotIndex, int32 NewSlotIndex) {}
