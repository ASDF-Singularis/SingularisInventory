#include "Widgets/SingularisPocketWidget.h"

void USingularisPocketWidget::OnPocketRefresh_Implementation(
	int32 Capacity,
	const TArray<USingularisItem*>& Items,
	int32 SelectedSlotIndex
) {}

void USingularisPocketWidget::OnItemAdded_Implementation(int32 SlotIndex, USingularisItem* Item) {}

void USingularisPocketWidget::OnItemRemoved_Implementation(int32 SlotIndex, USingularisItem* Item) {}

void USingularisPocketWidget::OnSelectionChanged_Implementation(int32 OldSlotIndex, int32 NewSlotIndex) {}
